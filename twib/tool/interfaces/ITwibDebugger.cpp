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

#include "Protocol.hpp"
#include "common/Logger.hpp"
#include "common/ResultError.hpp"

#include<cstring>

namespace twili {
namespace twib {
namespace tool {

ITwibDebugger::ITwibDebugger(std::shared_ptr<RemoteObject> obj) : obj(obj) {
	
}

std::tuple<nx::MemoryInfo, nx::PageInfo> ITwibDebugger::QueryMemory(uint64_t addr) {
	nx::MemoryInfo mi;
	nx::PageInfo pi;
	
	LogMessage(Debug, "ITwibDebugger::QueryMemory(0x%lx)", addr);
	
	obj->SendSmartSyncRequest(
		CommandID::QUERY_MEMORY,
		in<uint64_t>(addr),
		out<nx::MemoryInfo>(mi),
		out<nx::PageInfo>(pi));

	LogMessage(Debug, "  => {");
	LogMessage(Debug, "       base_addr: 0x%lx,", mi.base_addr);
	LogMessage(Debug, "       size: 0x%lx,", mi.size);
	LogMessage(Debug, "       memory_type: 0x%x,", mi.memory_type);
	LogMessage(Debug, "       memory_attribute: 0x%x,", mi.memory_attribute);
	LogMessage(Debug, "       permission: 0x%x,", mi.permission);
	LogMessage(Debug, "       device_ref_count: 0x%x,", mi.device_ref_count);
	LogMessage(Debug, "       ipc_ref_count: 0x%x }", mi.ipc_ref_count);
	
	return std::make_tuple(mi, pi);
}

std::vector<uint8_t> ITwibDebugger::ReadMemory(uint64_t addr, uint64_t size) {
	std::vector<uint8_t> bytes;

	LogMessage(Debug, "ITwibDebugger::ReadMemory(0x%lx, 0x%lx)", addr, size);
	
	obj->SendSmartSyncRequest(
		CommandID::READ_MEMORY,
		in<uint64_t>(addr),
		in<uint64_t>(size),
		out<std::vector<uint8_t>>(bytes));

	LogMessage(Debug, "  => OK");
	
	return bytes;
}

void ITwibDebugger::WriteMemory(uint64_t addr, std::vector<uint8_t> &bytes) {
	LogMessage(Debug, "ITwibDebugger::WriteMemory(0x%lx, 0x%lx)", bytes.size());
	
	obj->SendSmartSyncRequest(
		CommandID::WRITE_MEMORY,
		in<uint64_t>(addr),
		in<std::vector<uint8_t>>(std::move(bytes)));

	LogMessage(Debug, " => OK");
}

std::optional<nx::DebugEvent> ITwibDebugger::GetDebugEvent() {
	nx::DebugEvent event;
	
	LogMessage(Debug, "ITwibDebugger::GetDebugEvent()");
	
	uint32_t r = obj->SendSmartSyncRequestWithoutAssert(
		CommandID::GET_DEBUG_EVENT,
		out<nx::DebugEvent>(event));
	if(r == 0x8c01) {
		LogMessage(Debug, " => none");
		return std::nullopt;
	}
	if(r != 0) {
		LogMessage(Debug, " => error (0x%x)", r);
		throw ResultError(r);
	}

	static const char *event_type_names[] = {"AttachProcess", "AttachThread", "ExitProcess", "ExitThread", "Exception"};
	if((int) event.event_type < 5) {
		LogMessage(Debug, " => event (%s)", event_type_names[(int) event.event_type]);
	} else {
		LogMessage(Debug, "  => event (unknown %d)", (int) event.event_type);
	}
	
	return event;
}

ThreadContext ITwibDebugger::GetThreadContext(uint64_t thread_id) {
	ThreadContext tc;
	
	LogMessage(Debug, "ITwibDebugger::GetThreadContext(0x%lx)", thread_id);
	
	obj->SendSmartSyncRequest(
		CommandID::GET_THREAD_CONTEXT,
		in<uint64_t>(thread_id),
		out<ThreadContext>(tc));

	LogMessage(Debug, " => OK");
	return tc;
}

void ITwibDebugger::SetThreadContext(uint64_t thread_id, ThreadContext tc) {
	LogMessage(Debug, "ITwibDebugger::SetThreadContext(0x%lx)", thread_id);
	
	obj->SendSmartSyncRequest(
		CommandID::SET_THREAD_CONTEXT,
		in<uint64_t>(thread_id),
		in<uint32_t>(15), // gprs + pc + sp + cpsr + fpu + fpcr/fpsr
		in<ThreadContext>(tc));
	
	LogMessage(Debug, " => OK");
}

void ITwibDebugger::ContinueDebugEvent(uint32_t flags, std::vector<uint64_t> thread_ids) {
	LogMessage(Debug, "ITwibDebugger::ContinueDebugEvent(0x%x) {", flags);
	for(uint64_t tid : thread_ids) {
		LogMessage(Debug, "  thread 0x%lx,", tid);
	}
	LogMessage(Debug, "}");
	
	obj->SendSmartSyncRequest(
		CommandID::CONTINUE_DEBUG_EVENT,
		in<uint32_t>(flags),
		in<std::vector<uint64_t>>(thread_ids));

	LogMessage(Debug, " => OK");
}

void ITwibDebugger::BreakProcess() {
	LogMessage(Debug, "ITwibDebugger::BreakProcess()");
	obj->SendSmartSyncRequest(CommandID::BREAK_PROCESS);
	LogMessage(Debug, " => OK");
}

uint64_t ITwibDebugger::GetTargetEntry() {
	uint64_t e;

	LogMessage(Debug, "ITwibDebugger::GetTargetEntry()");
	
	obj->SendSmartSyncRequest(
		CommandID::GET_TARGET_ENTRY,
		out<uint64_t>(e));

	LogMessage(Debug, " => 0x%lx", e);
	
	return e;
}

void ITwibDebugger::AsyncWait(std::function<void(uint32_t)> &&func) {
	LogMessage(Debug, "ITwibDebugger::AsyncWait() => ?");
	obj->SendSmartRequest(
		CommandID::WAIT_EVENT,
		std::move(func));
}

void ITwibDebugger::LaunchDebugProcess() {
	LogMessage(Debug, "ITwibDebugger::LaunchDebugProcess()");
	obj->SendSmartSyncRequest(
		CommandID::LAUNCH_DEBUG_PROCESS);
	LogMessage(Debug, " => OK");
}

std::vector<nx::LoadedModuleInfo> ITwibDebugger::GetNsoInfos() {
	std::vector<nx::LoadedModuleInfo> infos;
	
	LogMessage(Debug, "ITwibDebugger::GetNsoInfos()");
	
	obj->SendSmartSyncRequest(
		CommandID::GET_NSO_INFOS,
		out(infos));
	
	LogMessage(Debug, " => {");
	for(nx::LoadedModuleInfo &lmi : infos) {
		LogMessage(Debug, "      {");
		
		char build_id[0x41];
		for(int i = 0; i < 0x20; i++) {
			snprintf(build_id + i*2, 3, "%02x", lmi.build_id[i]);
		}
		
		LogMessage(Debug, "        build_id: %s", build_id);
		LogMessage(Debug, "        base_addr: 0x%lx", lmi.base_addr);
		LogMessage(Debug, "        size: 0x%lx", lmi.size);
		LogMessage(Debug, "      },");
	}
	LogMessage(Debug, "    }");
	
	return infos;
}

std::vector<nx::LoadedModuleInfo> ITwibDebugger::GetNroInfos() {
	std::vector<nx::LoadedModuleInfo> infos;

	LogMessage(Debug, "ITwibDebugger::GetNsoInfos()");
	
	obj->SendSmartSyncRequest(
		CommandID::GET_NRO_INFOS,
		out(infos));

	LogMessage(Debug, " => {");
	for(nx::LoadedModuleInfo &lmi : infos) {
		LogMessage(Debug, "      {");
		
		char build_id[0x41];
		for(int i = 0; i < 0x20; i++) {
			snprintf(build_id + i*2, 3, "%02x", lmi.build_id[i]);
		}
		
		LogMessage(Debug, "        build_id: %s", build_id);
		LogMessage(Debug, "        base_addr: 0x%lx", lmi.base_addr);
		LogMessage(Debug, "        size: 0x%lx", lmi.size);
		LogMessage(Debug, "      },");
	}
	LogMessage(Debug, "    }");

	return infos;
}

} // namespace tool
} // namespace twib
} // namespace twili
