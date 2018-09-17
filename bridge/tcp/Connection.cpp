#include "TCPBridge.hpp"

#include<libtransistor/ipc/bsd.h>

#include "err.hpp"
#include "bridge/Object.hpp"

namespace twili {
namespace bridge {
namespace tcp {

TCPBridge::Connection::Connection(TCPBridge &bridge, util::Socket &&socket) :
	bridge(bridge),
	socket(std::move(socket)) {
	printf("constructing connection\n");
	objects.insert(std::pair<uint32_t, std::shared_ptr<bridge::Object>>(0, bridge.object_zero));
}

void TCPBridge::Connection::PumpInput() {
	std::tuple<uint8_t*, size_t> target = in_buffer.Reserve(8192);
	ssize_t r = bsd_recv(socket.fd, (void*) std::get<0>(target), std::get<1>(target), 0);
	printf("pumped in 0x%lx bytes\n", r);
	if(r <= 0) {
		deletion_flag = true;
		return;
	} else {
		in_buffer.MarkWritten(r);
	}
}

void TCPBridge::Connection::Process() {
	while(in_buffer.ReadAvailable() > 0) {
		if(!has_current_mh) {
			if(in_buffer.Read(current_mh)) {
				printf("read header\n");
				has_current_mh = true;
				current_payload.Clear();
				has_current_payload = false;
			} else {
				in_buffer.Reserve(sizeof(protocol::MessageHeader));
			}
		}

		if(!has_current_payload) {
			if(in_buffer.Read(current_payload, current_mh.payload_size)) {
				printf("read payload\n");
				has_current_payload = true;
				current_object_ids.Clear();
			} else {
				in_buffer.Reserve(current_mh.payload_size);
				return;
			}
		}

		if(in_buffer.Read(current_object_ids, current_mh.object_count * sizeof(uint32_t))) {
			// we've read all the components of the message
			printf("read entire message\n");
			ProcessCommand();
			has_current_mh = false;
			has_current_payload = false;
		} else {
			in_buffer.Reserve(current_mh.object_count * sizeof(uint32_t));
			return;
		}
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
