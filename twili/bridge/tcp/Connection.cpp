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

#include "TCPBridge.hpp"

#include<libtransistor/ipc/bsd.h>

#include<mutex>

#include "../../MutexShim.hpp"
#include "../Object.hpp"

#include "err.hpp"

namespace twili {
namespace bridge {
namespace tcp {

TCPBridge::Connection::Connection(TCPBridge &bridge, util::Socket &&socket) :
	bridge(bridge),
	socket(std::move(socket)) {
	objects.insert(std::pair<uint32_t, std::shared_ptr<bridge::Object>>(0, bridge.object_zero));
}

void TCPBridge::Connection::PumpInput() {
	std::tuple<uint8_t*, size_t> target = in_buffer.Reserve(8192);
	ssize_t r = bsd_recv(socket.fd, (void*) std::get<0>(target), std::get<1>(target), 0);
	if(r <= 0) {
		deletion_flag = true;
		return;
	} else {
		in_buffer.MarkWritten(r);
	}
}

void TCPBridge::Connection::Process() {
	while(!deletion_flag && in_buffer.ReadAvailable() > 0) {
		if(!has_current_mh) {
			if(in_buffer.Read(current_mh)) {
				has_current_mh = true;
				payload_size = 0;
				payload_buffer.Clear();
				has_current_payload = false;
				
				// pick command handler
				Synchronize(Task::BeginProcessingCommand);
			} else {
				in_buffer.Reserve(sizeof(protocol::MessageHeader));
			}
		}

		if(!has_current_payload) {
			size_t payload_avail = in_buffer.ReadAvailable();
			if(payload_avail > current_mh.payload_size - payload_size) {
				payload_avail = current_mh.payload_size - payload_size;
			}
			in_buffer.Read(payload_buffer, payload_avail);
			payload_size+= payload_avail;

			Synchronize(Task::FlushReceiveBuffer);
			
			if(payload_size == current_mh.payload_size) {
				Synchronize(Task::FinalizeCommand);
				has_current_mh = false;
				has_current_payload = false;
			} else {
				return;
			}
		}
	}
}

void TCPBridge::Connection::Synchronize(Task task) {
	util::MutexShim shim(bridge.request_processing_mutex);
	std::unique_lock<util::MutexShim> lock(shim);
	
	this->pending_task = task;
	// request that the main thread service us
	bridge.request_processing_connection = shared_from_this();
	bridge.request_processing_signal_wh->Signal();
	while(this->pending_task != Task::Idle) {
		trn_condvar_wait(&bridge.request_processing_condvar, &bridge.request_processing_mutex, -1);
	}
}

void TCPBridge::Connection::Synchronized() {
	switch(pending_task) {
	case Task::BeginProcessingCommand:
		BeginProcessingCommandImpl();
		break;
	case Task::FlushReceiveBuffer:
		try {
			current_handler->FlushReceiveBuffer(payload_buffer);
		} catch(trn::ResultError &e) {
			if(!current_state->has_begun) {
				ResponseOpener opener(current_state);
				opener.RespondError(e.code);
			} else {
				printf("TCPConnection: dropped error during FlushReceiveBuffer: 0x%x\n", e.code.code);
				ResetHandler();
			}
		}
		break;
	case Task::FinalizeCommand:
		try {
			current_object->FinalizeCommand(payload_buffer);
			ResetHandler();
		} catch(trn::ResultError &e) {
			if(!current_state->has_begun) {
				ResponseOpener opener(current_state);
				opener.RespondError(e.code);
			} else {
				printf("TCPConnection: dropped error during Finalize: 0x%x\n", e.code.code);
				ResetHandler();
			}
		}
		break;
	case Task::Idle:
		printf("TCPConnection: synchronized on idle task?\n");
		break;
	}
	pending_task = Task::Idle;
}

void TCPBridge::Connection::BeginProcessingCommandImpl() {
	current_state = std::make_shared<Connection::ResponseState>(shared_from_this(), current_mh.client_id, current_mh.tag);
	ResponseOpener opener(current_state);
	auto i = objects.find(current_mh.object_id);
	if(i == objects.end()) {
		opener.BeginError(TWILI_ERR_PROTOCOL_UNRECOGNIZED_OBJECT).Finalize();
		return;
	}

	// check for a close object request
	if(current_mh.command_id == 0xffffffff) {
		printf("got close command for %d\n", current_mh.object_id);
		if(current_mh.object_id == 0) {
			// for USBBridge, this is intended to cleanup objects left by another
			// twibd. we get to use TCP connections instead.
		} else {
			objects.erase(current_mh.object_id);
		}
		opener.RespondOk();
		return;
	}

	try {
		current_object = i->second;
		current_handler = current_object->OpenRequest(current_mh.command_id, current_mh.payload_size, opener);
	} catch(trn::ResultError &e) {
		if(!current_state->has_begun) {
			opener.RespondError(e.code);
		} else {
			throw e;
		}
	}
}

void TCPBridge::Connection::ResetHandler() {
	current_object.reset();
	current_handler = DiscardingRequestHandler::GetInstance();
}

} // namespace tcp
} // namespace bridge
} // namespace twili
