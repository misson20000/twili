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
				SignalInput();
				return nullptr;
			}
		}

		if(!has_current_payload) {
			if(in_buffer.Read(current_rq.payload, current_rq.mh.payload_size)) {
				has_current_payload = true;
				current_rq.object_ids.Clear();
			} else {
				in_buffer.Reserve(current_rq.mh.payload_size);
				SignalInput();
				return nullptr;
			}
		}

		if(in_buffer.Read(current_rq.object_ids, current_rq.mh.object_count * sizeof(uint32_t))) {
			has_current_mh = false;
			has_current_payload = false;
			return &current_rq;
		} else {
			in_buffer.Reserve(current_rq.mh.object_count * sizeof(uint32_t));
			SignalInput();
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
	SignalOutput();
}

} // namespace twibc
} // namespace twili
