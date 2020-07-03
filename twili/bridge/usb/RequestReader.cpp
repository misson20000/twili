//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2018 misson20000 <xenotoad@xenotoad.net>
//
// This file is part of Twili.
//
// Twili is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Twili is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Twili.  If not, see <http://www.gnu.org/licenses/>.
//

typedef bool _Bool;
#include<atomic>
#include<memory>
#include<stdint.h>
#include<stdatomic.h>
#include<libtransistor/alloc_pages.h>
#include<libtransistor/err.h>
#include<libtransistor/util.h>
#include<libtransistor/svc.h>

#include "../../twili.hpp"
#include "../Object.hpp"
#include "../ResponseOpener.hpp"
#include "USBBridge.hpp"

#include "err.hpp"

namespace twili {
namespace bridge {
namespace usb {

using trn::ResultCode;
using trn::ResultError;

USBBridge::RequestReader::RequestReader(USBBridge *bridge) : bridge(bridge) {
}

USBBridge::RequestReader::~RequestReader() {
}

void USBBridge::RequestReader::Begin() {
	meta_completion_wait =
		bridge->twili->event_waiter.Add(
			bridge->endpoint_request_meta->completion_event, [this]() {
				try {
					this->MetadataTransactionCompleted();
					this->PostMetaBuffer();
					return true;
				} catch(ResultError &e) {
					bridge->ResetInterface();
					return false;
				} catch(std::exception &e) {
					bridge->ResetInterface();
					return false;
				}
			});
		
	data_completion_wait =
		bridge->twili->event_waiter.Add(
			bridge->endpoint_request_data->completion_event, [this]() {
				try {
					this->DataTransactionCompleted();
					return true;
				} catch(ResultError &e) {
					bridge->ResetInterface();
					return false;
				} catch(std::exception &e) {
					bridge->ResetInterface();
					return false;
				}
			});

	PostMetaBuffer();
}

void USBBridge::RequestReader::PostMetaBuffer() {
	auto r = bridge->endpoint_request_meta->PostBufferAsync(
		bridge->request_meta_buffer.data,
		sizeof(protocol::MessageHeader));
	if(r) {
		meta_urb_id = *r;
	} else {
		bridge->ResetInterface();
	}
}

void USBBridge::RequestReader::PostDataBuffer() {
	size_t size = bridge->request_data_buffer.size;
	if(current_header.payload_size - payload_size < size) {
		size = current_header.payload_size - payload_size;
	}

	auto r = bridge->endpoint_request_data->PostBufferAsync(bridge->request_data_buffer.data, size);
	if(r) {
		data_urb_id = *r;
	} else {
		bridge->ResetInterface();
	}
}

void USBBridge::RequestReader::PostObjectBuffer() {
	size_t size = current_header.object_count * sizeof(uint32_t);
	if(size > bridge->request_data_buffer.size) {
		printf("Too many object IDs in request\n");
		bridge->ResetInterface();
		return;
	}
	
	auto r = bridge->endpoint_request_data->PostBufferAsync(bridge->request_data_buffer.data, size);
	if(r) {
		object_urb_id = *r;
	} else {
		bridge->ResetInterface();
	}
}

void USBBridge::RequestReader::MetadataTransactionCompleted() {
	usb_ds_report_t report;
	auto entry = USBBridge::FindReport(bridge->endpoint_request_meta, report, meta_urb_id);
	if(entry->urb_status != 3) {
		printf("Meta URB status (%d) != 3\n", entry->urb_status);
		bridge->ResetInterface();
		return;
	}
	if(entry->transferred_size == 0) {
		return;
	}
	if(entry->transferred_size != sizeof(protocol::MessageHeader)) {
		bridge->ResetInterface();
		return;
	}
	memcpy(&current_header, bridge->request_meta_buffer.data, sizeof(protocol::MessageHeader));
	if(bridge->twili->config.logging_verbosity >= 1) {
		printf("got header, object 0x%x, command %u, payload size 0x%lx\n", current_header.object_id, current_header.command_id, current_header.payload_size);
	}

	payload_size = 0;
	object_ids.clear();
	payload_buffer.Clear();
	ResetHandler();
	
	// pick command handler
	BeginProcessingCommand();
	
	if(current_header.payload_size > 0) {
		PostDataBuffer();
	} else if(current_header.object_count > 0) {
		PostObjectBuffer();
	} else {
		FinalizeCommand();
	}
}

void USBBridge::RequestReader::DataTransactionCompleted() {
	usb_ds_report_t report;

	// we've finished receiving the payload, so this must be object IDs
	if(payload_size == current_header.payload_size) {
		auto entry = USBBridge::FindReport(bridge->endpoint_request_data, report, object_urb_id);
		if(entry->urb_status != 3) {
			printf("Object URB status (%d) != 3\n", entry->urb_status);
			bridge->ResetInterface();
			return;
		}
		if(entry->transferred_size != current_header.object_count * sizeof(uint32_t)) {
			printf("Didn't receive enough object IDs\n");
			bridge->ResetInterface();
			return;
		}
		
		std::copy( // copy object IDs
			((uint32_t*) bridge->request_data_buffer.data),
			((uint32_t*) bridge->request_data_buffer.data) + current_header.object_count,
			object_ids.insert(object_ids.end(), current_header.object_count, 0));
		FinalizeCommand();

		return;
	}
	
	auto entry = USBBridge::FindReport(bridge->endpoint_request_data, report, data_urb_id);
	if(entry->urb_status != 3) {
		printf("Data URB status (%d) != 3\n", entry->urb_status);
		bridge->ResetInterface();
		return;
	}
	if(payload_size + entry->transferred_size > current_header.payload_size) {
		printf("Overshot payload size\n");
		bridge->ResetInterface();
		return;
	}

	payload_size+= entry->transferred_size;
	
	try {
		// fill input buffer with data from USB
		payload_buffer.Write(bridge->request_data_buffer.data, entry->transferred_size);

		// pass to request handler
		current_handler->FlushReceiveBuffer(payload_buffer);
	} catch(trn::ResultError &e) {
		printf("USBRequestReader: Somebody is still throwing exceptions!\n");
		twili::Abort(e);
	} catch(std::bad_alloc &e) {
		if(!current_state->has_begun) {
			printf("USBRequestReader: ran out of memory. trying to signal this to user...\n");
			ResponseOpener opener(current_state);
			opener.RespondError(LIBTRANSISTOR_ERR_OUT_OF_MEMORY);
		} else {
			printf("USBRequestReader: dropped std::bad_alloc during FlushReceiveBuffer\n");
			bridge->ResetInterface();
			return;
		}
		CleanupCommand();
	}
	
	if(payload_size < current_header.payload_size) {
		PostDataBuffer();
	} else if(current_header.object_count > 0) {
		PostObjectBuffer();
	} else {
		FinalizeCommand();
	}
}

void USBBridge::RequestReader::BeginProcessingCommand() {
	current_state = std::make_shared<USBBridge::ResponseState>(*bridge, current_header.client_id, current_header.tag);
	ResponseOpener opener(current_state);
	auto i = bridge->objects.find(current_header.object_id);
	if(i == bridge->objects.end()) {
		opener.RespondError(TWILI_ERR_PROTOCOL_UNRECOGNIZED_OBJECT);
		return;
	}
	
	// check for a close object request
	if(current_header.command_id == 0xffffffff) {
		printf("got close command for %d\n", current_header.object_id);
		if(current_header.object_id == 0) {
			// closing object 0 closes everything except object 0
			for(auto i = bridge->objects.begin(); i != bridge->objects.end();) {
				if(i->first != 0) { // if object id !=0
					printf("  closing %d\n", i->first);
					i = bridge->objects.erase(i); // delete it
				} else {
					i++;
				}
			}
		} else {
			// delete it
			bridge->objects.erase(current_header.object_id);
		}
		opener.RespondOk();
		return;
	}
	
	try {
		current_object = i->second;
		current_handler = current_object->OpenRequest(current_header.command_id, current_header.payload_size, opener);
	} catch(trn::ResultError &e) {
		printf("USBRequestReader: Somebody is still throwing exceptions!\n");
		twili::Abort(e);
	} catch(std::bad_alloc &e) {
		printf("USBRequestReader: out of memory while opening request\n");
		twili::Abort(LIBTRANSISTOR_ERR_OUT_OF_MEMORY);
	}
}

void USBBridge::RequestReader::FinalizeCommand() {
	try {
		current_handler->Finalize(payload_buffer);
		CleanupCommand();
	} catch(ResultError &e) {
		printf("USBRequestReader: Somebody is still throwing exceptions!\n");
		twili::Abort(e);
	}
}

void USBBridge::RequestReader::CleanupCommand() {
	if(current_object) {
		current_object->FinalizeCommand();
		current_object.reset();
	}
	ResetHandler();
}

void USBBridge::RequestReader::ResetHandler() {
	current_state.reset();
	current_handler = DiscardingRequestHandler::GetInstance();
}

} // namespace usb
} // namespace bridge
} // namespace twili
