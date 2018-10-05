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
				current_payload.Clear();
				has_current_payload = false;
			} else {
				in_buffer.Reserve(sizeof(protocol::MessageHeader));
			}
		}

		if(!has_current_payload) {
			if(in_buffer.Read(current_payload, current_mh.payload_size)) {
				has_current_payload = true;
				current_object_ids.Clear();
			} else {
				in_buffer.Reserve(current_mh.payload_size);
				return;
			}
		}

		if(in_buffer.Read(current_object_ids, current_mh.object_count * sizeof(uint32_t))) {
			// we've read all the components of the message
			SynchronizeCommand();
			has_current_mh = false;
			has_current_payload = false;
		} else {
			in_buffer.Reserve(current_mh.object_count * sizeof(uint32_t));
			return;
		}
	}
}

/*
 * Request that the main thread process the message we've just read,
 * and then block until the main thread finishes. I know that this
 * isn't very parallel, but I'm only using threads here to work around
 * bad socket synchronization primitives so I don't really care.
 * The faster we can block this (the I/O) thread and keep it from breaking
 * things, the better.
 */
void TCPBridge::Connection::SynchronizeCommand() {
	util::MutexShim shim(bridge.request_processing_mutex);
	std::unique_lock<util::MutexShim> lock(shim);
	
	processing_message = true;
	// request that the main thread service us
	bridge.request_processing_connection = shared_from_this();
	bridge.request_processing_signal_wh->Signal();
	while(processing_message) {
		trn_condvar_wait(&bridge.request_processing_condvar, &bridge.request_processing_mutex, -1);
	}
}

void TCPBridge::Connection::ProcessCommand() {
	std::shared_ptr<ResponseState> state = std::make_shared<Connection::ResponseState>(shared_from_this(), current_mh.client_id, current_mh.tag);
	ResponseOpener opener(state);
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
		opener.BeginOk().Finalize();
		return;
	}

	try {
		i->second->HandleRequest(current_mh.command_id, current_payload.GetData(), opener);
	} catch(trn::ResultError &e) {
		if(!state->has_begun) {
			opener.BeginError(e.code).Finalize();
		} else {
			throw e;
		}
	}
}

} // namespace tcp
} // namespace bridge
} // namespace twili
