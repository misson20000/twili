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

#include "ITwibDebugger.hpp"

#include<cstring>
#include "Protocol.hpp"
#include "ResultError.hpp"

namespace twili {
namespace twib {

ITwibDebugger::ITwibDebugger(std::shared_ptr<RemoteObject> obj) : obj(obj) {
	
}

std::vector<uint8_t> ITwibDebugger::ReadMemory(uint64_t addr, uint64_t size) {
	struct {
		uint64_t addr;
		uint64_t size;
	} rq = {addr, size};
	uint8_t *bytes = (uint8_t*) &rq;
	return obj->SendSyncRequest(protocol::ITwibDebugger::Command::READ_MEMORY, std::vector<uint8_t>(bytes, bytes + sizeof(rq))).payload;
}

std::optional<nx::DebugEvent> ITwibDebugger::GetDebugEvent() {
	nx::DebugEvent event;
	memset(&event, 0, sizeof(event));
	Response rs = obj->SendSyncRequestWithoutAssert((uint32_t) protocol::ITwibDebugger::Command::GET_DEBUG_EVENT);
	if(rs.result_code == 0x8c01) {
		return std::nullopt;
	}
	if(rs.result_code != 0) {
		throw ResultError(rs.result_code);
	}
	std::vector<uint8_t> payload = rs.payload;
	memcpy(&event, payload.data(), payload.size());
	return event;
}

std::vector<uint64_t> ITwibDebugger::GetThreadContext(uint64_t thread_id) {
	uint8_t *tid_bytes = (uint8_t*) &thread_id;
	std::vector<uint8_t> payload = obj->SendSyncRequest(protocol::ITwibDebugger::Command::GET_THREAD_CONTEXT, std::vector<uint8_t>(tid_bytes, tid_bytes + sizeof(thread_id))).payload;
	return std::vector<uint64_t>((uint64_t*) payload.data(), (uint64_t*) (payload.data() + payload.size()));
}

void ITwibDebugger::ContinueDebugEvent(uint32_t flags, std::vector<uint64_t> thread_ids) {
	std::vector<uint8_t> data;
	data.resize(4 * (1 + 2 * thread_ids.size()));
	std::memcpy(&data[0], &flags, sizeof(flags));
	std::memcpy(&data[4], thread_ids.data(), thread_ids.size() * sizeof(uint64_t));
	obj->SendSyncRequest(protocol::ITwibDebugger::Command::CONTINUE_DEBUG_EVENT, data);
}

void ITwibDebugger::BreakProcess() {
	obj->SendSyncRequest(protocol::ITwibDebugger::Command::BREAK_PROCESS);
}

} // namespace twib
} // namespace twili
