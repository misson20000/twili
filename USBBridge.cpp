typedef bool _Bool;
#include<atomic>
#include<memory>
#include<stdint.h>
#include<stdatomic.h>
#include<libtransistor/alloc_pages.h>
#include<libtransistor/err.h>
#include<libtransistor/util.h>
#include<libtransistor/svc.h>

#include "twili.hpp"
#include "BridgeObject.hpp"
#include "USBBridge.hpp"
#include "err.hpp"

namespace twili {
namespace usb {

static usb_endpoint_descriptor_t endpoint_in_descriptor = {
	.bLength = sizeof(usb_endpoint_descriptor_t),
	.bDescriptorType = TRN_USB_DT_ENDPOINT,
	.bEndpointAddress = TRN_USB_ENDPOINT_IN,
	.bmAttributes = TRN_USB_TRANSFER_TYPE_BULK,
	.wMaxPacketSize = 0x200,
	.bInterval = 0,
};

static usb_endpoint_descriptor_t endpoint_out_descriptor = {
	.bLength = sizeof(usb_endpoint_descriptor_t),
	.bDescriptorType = TRN_USB_DT_ENDPOINT,
	.bEndpointAddress = TRN_USB_ENDPOINT_OUT,
	.bmAttributes = TRN_USB_TRANSFER_TYPE_BULK,
	.wMaxPacketSize = 0x200,
	.bInterval = 0,
};

static usb_interface_descriptor_t interface_descriptor = {
	.bLength = 0x9,
	.bDescriptorType = TRN_USB_DT_INTERFACE,
	.bInterfaceNumber = USB_DS_INTERFACE_NUMBER_AUTO,
	.bAlternateSetting = 0x00,
	.bNumEndpoints = 0x00,
	.bInterfaceClass = 0xFF,
	.bInterfaceSubClass = 0x01,
	.bInterfaceProtocol = 0x00,
	.iInterface = 0x00
};

using trn::ResultCode;
using trn::ResultError;

static const size_t TRANSFER_BUFFER_SIZE = 0x8000;

USBBridge::USBBridge(Twili *twili, std::shared_ptr<bridge::Object> object_zero) :
	twili(twili),
	ds(ResultCode::AssertOk(trn::service::usb::ds::DS::Initialize(2))),
	usb_state_change_event(ResultCode::AssertOk(ds.GetStateChangeEvent())),
	request_reader(this),
	request_meta_buffer(0x1000),
	response_meta_buffer(0x1000),
	request_data_buffer(TRANSFER_BUFFER_SIZE),
	response_data_buffer(TRANSFER_BUFFER_SIZE) {
	
	interface = ResultCode::AssertOk(
		ds.GetInterface(interface_descriptor, "twili_bridge"));
	endpoint_response_meta = ResultCode::AssertOk(
		interface->GetEndpoint(endpoint_in_descriptor)); // into host
	endpoint_request_meta = ResultCode::AssertOk(
		interface->GetEndpoint(endpoint_out_descriptor)); // out from host
	endpoint_response_data = ResultCode::AssertOk(
		interface->GetEndpoint(endpoint_in_descriptor)); // into host
	endpoint_request_data = ResultCode::AssertOk(
		interface->GetEndpoint(endpoint_out_descriptor)); // out from host
	ResultCode::AssertOk(interface->Enable());

	objects.insert(std::pair<uint32_t, std::shared_ptr<bridge::Object>>(0, object_zero));

	state_change_wait = twili->event_waiter.Add(usb_state_change_event, [this]() {
			return USBStateChangeCallback();
		});
}

usb_ds_report_entry_t *USBBridge::FindReport(std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint, usb_ds_report_t &report, uint32_t urb_id) {
	report = ResultCode::AssertOk(endpoint->GetReportData());
	usb_ds_report_entry_t *entry = nullptr;
	for(uint32_t i = 0; i < report.entry_count; i++) {
		if(report.entries[i].urb_id == urb_id) {
			return &report.entries[i];
		}
	}
	throw ResultError(TWILI_ERR_FATAL_USB_TRANSFER);
}

void USBBridge::PostBufferSync(std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint, uint8_t *buffer, size_t size) {
	uint32_t urb_id = ResultCode::AssertOk(endpoint->PostBufferAsync(buffer, size));
	ResultCode::AssertOk(endpoint->completion_event.WaitSignal(30000000000));
	ResultCode::AssertOk(endpoint->completion_event.ResetSignal());
	
	usb_ds_report_t report;
	auto entry = FindReport(endpoint, report, urb_id);
	if(entry->urb_status != 3) {
		throw ResultError(TWILI_ERR_USB_TRANSFER);
	}
	if(entry->transferred_size < size) {
		printf("[USBB] didn't send all bytes, posting again...\n");
		PostBufferSync(endpoint, buffer + entry->transferred_size, size - entry->transferred_size);
	}
}

bool USBBridge::USBStateChangeCallback() {
	if(ResultCode::AssertOk(ds.GetState()) == trn::service::usb::ds::State::INITIALIZED) {
		printf("finished USB bringup\n");
		try {
			request_reader.Begin();
		} catch(ResultError &e) { // until libtransistor 2.0.1, we have to live with ResultError not inheriting publicly from std::runtime_error
			ResetInterface();
		} catch(std::exception &e) {
			ResetInterface();
		}
	} else {
		// USB has gone down
	}
	return true;
}

void USBBridge::ResetInterface() {
	interface->Disable();
	interface->Enable();
}

USBBridge::~USBBridge() {
}

USBBridge::RequestReader::RequestReader(twili::usb::USBBridge *bridge) : bridge(bridge) {
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
	meta_urb_id = ResultCode::AssertOk(
		bridge->endpoint_request_meta->PostBufferAsync(
			bridge->request_meta_buffer.data,
			sizeof(protocol::MessageHeader)));
}

void USBBridge::RequestReader::PostDataBuffer() {
	size_t size = bridge->request_data_buffer.size;
	if(current_header.payload_size - current_payload.size() < size) {
		size = current_header.payload_size - current_payload.size();
	}

	data_urb_id = ResultCode::AssertOk(
		bridge->endpoint_request_data->PostBufferAsync(
			bridge->request_data_buffer.data, size));
}

void USBBridge::RequestReader::PostObjectBuffer() {
	size_t size = current_header.object_count * sizeof(uint32_t);
	if(size > bridge->request_data_buffer.size) {
		printf("Too many object IDs in request\n");
		throw ResultError(TWILI_ERR_FATAL_USB_TRANSFER);
	}
	
	object_urb_id = ResultCode::AssertOk(
		bridge->endpoint_request_data->PostBufferAsync(
			bridge->request_data_buffer.data, size));
}

void USBBridge::RequestReader::MetadataTransactionCompleted() {
	usb_ds_report_t report;
	auto entry = USBBridge::FindReport(bridge->endpoint_request_meta, report, meta_urb_id);
	if(entry->urb_status != 3) {
		printf("Meta URB status (%d) != 3\n", entry->urb_status);
		throw ResultError(TWILI_ERR_FATAL_USB_TRANSFER);
	}
	if(entry->transferred_size == 0) {
		return;
	}
	if(entry->transferred_size != sizeof(protocol::MessageHeader)) {
		throw ResultError(TWILI_ERR_FATAL_USB_TRANSFER);
	}
	memcpy(&current_header, bridge->request_meta_buffer.data, sizeof(protocol::MessageHeader));
	printf("got header, command %u, payload size 0x%lx\n", current_header.command_id, current_header.payload_size);
	
	current_payload.clear();
	object_ids.clear();
	if(current_header.payload_size > 0) {
		current_payload.reserve(current_header.payload_size);
		PostDataBuffer();
	} else if(current_header.object_count > 0) {
		PostObjectBuffer();
	} else {
		ProcessCommand();
	}
}

void USBBridge::RequestReader::DataTransactionCompleted() {
	usb_ds_report_t report;

	// we've finished receiving the payload, so this must be object IDs
	if(current_payload.size() == current_header.payload_size) {
		auto entry = USBBridge::FindReport(bridge->endpoint_request_data, report, object_urb_id);
		if(entry->urb_status != 3) {
			printf("Object URB status (%d) != 3\n", entry->urb_status);
			throw ResultError(TWILI_ERR_FATAL_USB_TRANSFER);
		}
		if(entry->transferred_size != current_header.object_count * sizeof(uint32_t)) {
			printf("Didn't receive enough object IDs\n");
			throw ResultError(TWILI_ERR_FATAL_USB_TRANSFER);
			std::copy( // copy object IDs
				((uint32_t*) bridge->request_data_buffer.data),
				((uint32_t*) bridge->request_data_buffer.data) + current_header.object_count,
				object_ids.insert(object_ids.end(), current_header.object_count, 0));
			ProcessCommand();
		}
		return;
	}
	
	auto entry = USBBridge::FindReport(bridge->endpoint_request_data, report, data_urb_id);
	if(entry->urb_status != 3) {
		printf("Data URB status (%d) != 3\n", entry->urb_status);
		throw ResultError(TWILI_ERR_FATAL_USB_TRANSFER);
	}
	if(current_payload.size() + entry->transferred_size > current_header.payload_size) {
		printf("Overshot payload size\n");
		throw ResultError(TWILI_ERR_FATAL_USB_TRANSFER);
	}
	
	std::copy( // append data to current_payload
		bridge->request_data_buffer.data,
		bridge->request_data_buffer.data + entry->transferred_size,
		current_payload.insert(current_payload.end(), entry->transferred_size, 0));
	
	if(current_payload.size() < current_header.payload_size) {
		PostDataBuffer();
	} else if(current_header.object_count > 0) {
		PostObjectBuffer();
	} else {
		ProcessCommand();
	}
}

void USBBridge::RequestReader::ProcessCommand() {
	std::shared_ptr<ResponseState> state = std::make_shared<ResponseState>(*bridge, current_header.client_id, current_header.tag);
	ResponseOpener opener(state);
	auto i = bridge->objects.find(current_header.object_id);
	if(i == bridge->objects.end()) {
		opener.BeginError(TWILI_ERR_PROTOCOL_UNRECOGNIZED_OBJECT).Finalize();
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
		opener.BeginOk().Finalize();
		return;
	}
	
	try {
		i->second->HandleRequest(current_header.command_id, current_payload, opener);
	} catch(trn::ResultError &e) {
		if(!state->has_begun) {
			opener.BeginError(e.code).Finalize();
		} else {
			throw e;
		}
	}
}

USBBridge::ResponseState::ResponseState(USBBridge &bridge, uint32_t client_id, uint32_t tag) :
	bridge(bridge),
	client_id(client_id),
	tag(tag) {
	
}

void USBBridge::ResponseState::Finalize() {
	if(transferred_size != total_size) {
		throw ResultError(TWILI_ERR_BAD_RESPONSE);
	}
	if(objects.size() != object_count) {
		throw ResultError(TWILI_ERR_BAD_RESPONSE);
	}

	if(object_count > 0) {
		// send object IDs
		uint32_t *out = (uint32_t*) bridge.response_data_buffer.data;
		for(auto p : objects) {
			*(out++) = p->object_id;
		}
		USBBridge::PostBufferSync(bridge.endpoint_response_data, bridge.response_data_buffer.data, object_count * sizeof(uint32_t));
	}
}

USBBridge::ResponseOpener::ResponseOpener(std::shared_ptr<ResponseState> state) : state(state) {
}

USBBridge::ResponseWriter USBBridge::ResponseOpener::BeginError(ResultCode code, size_t payload_size, uint32_t object_count) {
	if(state->has_begun) {
		throw ResultError(TWILI_ERR_FATAL_USB_TRANSFER);
	}
	if(object_count * sizeof(uint32_t) > state->bridge.response_data_buffer.size) {
		throw ResultError(TWILI_ERR_FATAL_USB_TRANSFER);
	}
	
	protocol::MessageHeader hdr;
	hdr.client_id = state->client_id;
	hdr.result_code = code.code;
	hdr.tag = state->tag;
	hdr.payload_size = payload_size;
	hdr.object_count = object_count;
	state->has_begun = true;
	state->total_size = payload_size;
	state->object_count = object_count;
	
	memcpy(
		state->bridge.response_meta_buffer.data,
		&hdr,
		sizeof(hdr));
	
	USBBridge::PostBufferSync(
		state->bridge.endpoint_response_meta,
		state->bridge.response_meta_buffer.data,
		sizeof(hdr));

	ResponseWriter writer(state);
	return writer;
}

USBBridge::ResponseWriter USBBridge::ResponseOpener::BeginOk(size_t payload_size, uint32_t object_count) {
	return BeginError(ResultCode(0), payload_size, object_count);
}

USBBridge::ResponseWriter::ResponseWriter(std::shared_ptr<ResponseState> state) : state(state) {
}

size_t USBBridge::ResponseWriter::GetMaxTransferSize() {
	return state->bridge.response_data_buffer.size;
}

void USBBridge::ResponseWriter::Write(uint8_t *data, size_t size) {
	auto max_size = state->bridge.response_data_buffer.size;
	while(size > max_size) {
		Write(data, max_size);
		data+= max_size;
		size-= max_size;
	}
	memcpy(state->bridge.response_data_buffer.data, data, size);
	USBBridge::PostBufferSync(state->bridge.endpoint_response_data, state->bridge.response_data_buffer.data, size);
	state->transferred_size+= size;
}

void USBBridge::ResponseWriter::Write(std::string str) {
	Write((uint8_t*) str.data(), str.size());
}

uint32_t USBBridge::ResponseWriter::Object(std::shared_ptr<bridge::Object> object) {
	size_t index = state->objects.size();
	state->objects.push_back(object);
	return index;
}

void USBBridge::ResponseWriter::Finalize() {
	state->Finalize();
}

USBBuffer::USBBuffer(size_t size) : size(size) {
	data = (uint8_t*) alloc_pages(size, size, nullptr);
	if(data == NULL) {
		throw ResultError(LIBTRANSISTOR_ERR_OUT_OF_MEMORY);
	}
	auto r = ResultCode::ExpectOk(svcSetMemoryAttribute(data, size, 0x8, 0x8));
	if(!r) {
		free_pages(data);
		throw ResultError(r.error());
	}
}

USBBuffer::~USBBuffer() {
	ResultCode::AssertOk(svcSetMemoryAttribute(data, size, 0x0, 0x0));
	free_pages(data);
}

}
}
