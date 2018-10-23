#include "MessageConnection.hpp"

namespace twili {
namespace twibc {

MessageConnection::MessageConnection() {
}

MessageConnection::~MessageConnection() {
}

MessageConnection::Request *MessageConnection::Process() {
	while(in_buffer.ReadAvailable() > 0) {
		if(!has_current_mh) {
			if(in_buffer.Read(current_rq.mh)) {
				has_current_mh = true;
				current_rq.payload.Clear();
				has_current_payload = false;
			} else {
				in_buffer.Reserve(sizeof(protocol::MessageHeader));
				RequestInput();
				return nullptr;
			}
		}

		if(!has_current_payload) {
			if(in_buffer.Read(current_rq.payload, current_rq.mh.payload_size)) {
				has_current_payload = true;
				current_rq.object_ids.Clear();
			} else {
				in_buffer.Reserve(current_rq.mh.payload_size);
				RequestInput();
				return nullptr;
			}
		}

		if(in_buffer.Read(current_rq.object_ids, current_rq.mh.object_count * sizeof(uint32_t))) {
			has_current_mh = false;
			has_current_payload = false;
			return &current_rq;
		} else {
			in_buffer.Reserve(current_rq.mh.object_count * sizeof(uint32_t));
			RequestInput();
			return nullptr;
		}
	}
	return nullptr;
}

void MessageConnection::SendMessage(const protocol::MessageHeader &mh, const std::vector<uint8_t> &payload, const std::vector<uint32_t> &object_ids) {
	std::lock_guard<std::mutex> lock(out_buffer_mutex);
	out_buffer.Write(mh);
	out_buffer.Write(payload);
	out_buffer.Write(object_ids);
	RequestOutput();
}

} // namespace twibc
} // namespace twili
