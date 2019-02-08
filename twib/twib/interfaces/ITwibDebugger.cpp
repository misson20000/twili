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

std::tuple<nx::MemoryInfo, nx::PageInfo> ITwibDebugger::QueryMemory(uint64_t addr) {
	nx::MemoryInfo mi;
	nx::PageInfo pi;
	obj->SendSmartSyncRequest(
		CommandID::QUERY_MEMORY,
		in<uint64_t>(addr),
		out<nx::MemoryInfo>(mi),
		out<nx::PageInfo>(pi));
	return std::make_tuple(mi, pi);
}

std::vector<uint8_t> ITwibDebugger::ReadMemory(uint64_t addr, uint64_t size) {
	std::vector<uint8_t> bytes;
	obj->SendSmartSyncRequest(
		CommandID::READ_MEMORY,
		in<uint64_t>(addr),
		in<uint64_t>(size),
		out<std::vector<uint8_t>>(bytes));
	return bytes;
}

void ITwibDebugger::WriteMemory(uint64_t addr, std::vector<uint8_t> &bytes) {
	obj->SendSmartSyncRequest(
		CommandID::WRITE_MEMORY,
		in<uint64_t>(addr),
		in<std::vector<uint8_t>>(std::move(bytes)));
}

std::optional<nx::DebugEvent> ITwibDebugger::GetDebugEvent() {
	nx::DebugEvent event;
	uint32_t r = obj->SendSmartSyncRequestWithoutAssert(
		CommandID::GET_DEBUG_EVENT,
		out<nx::DebugEvent>(event));
	if(r == 0x8c01) {
		return std::nullopt;
	}
	if(r != 0) {
		throw ResultError(r);
	}
	return event;
}

std::vector<uint64_t> ITwibDebugger::GetThreadContext(uint64_t thread_id) {
	struct ThreadContext {
		uint64_t regs[100];
	} tc;
	obj->SendSmartSyncRequest(
		CommandID::GET_THREAD_CONTEXT,
		in<uint64_t>(thread_id),
		out<ThreadContext>(tc));
	return std::vector<uint64_t>(&tc.regs[0], &tc.regs[100]);
}

void ITwibDebugger::SetThreadContext(uint64_t thread_id, std::vector<uint64_t> regs) {
	struct ThreadContext {
		uint64_t regs[100];
	} tc;
	std::copy(regs.begin(), regs.end(), tc.regs);
	obj->SendSmartSyncRequest(
		CommandID::SET_THREAD_CONTEXT,
		in<uint64_t>(thread_id),
		in<uint32_t>(3), // gprs + pc + sp + cpsr
		in<ThreadContext>(tc));
}

void ITwibDebugger::ContinueDebugEvent(uint32_t flags, std::vector<uint64_t> thread_ids) {
	obj->SendSmartSyncRequest(
		CommandID::CONTINUE_DEBUG_EVENT,
		in<uint32_t>(flags),
		in<std::vector<uint64_t>>(thread_ids));
}

void ITwibDebugger::BreakProcess() {
	obj->SendSmartSyncRequest(CommandID::BREAK_PROCESS);
}

uint64_t ITwibDebugger::GetTargetEntry() {
	uint64_t e;
	obj->SendSmartSyncRequest(
		CommandID::GET_TARGET_ENTRY,
		out<uint64_t>(e));
	return e;
}

void ITwibDebugger::AsyncWait(std::function<void(uint32_t)> &&func) {
	obj->SendSmartRequest(
		CommandID::WAIT_EVENT,
		std::move(func));
}

} // namespace twib
} // namespace twili
