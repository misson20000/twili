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

#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/waiter.hpp>

#include "../Object.hpp"
#include "../ResponseOpener.hpp"
#include "../RequestHandler.hpp"

namespace twili {

class Twili;

namespace process {

class MonitoredProcess;

} // namespace process

namespace bridge {

class ITwibDebugger : public ObjectDispatcherProxy<ITwibDebugger> {
 public:
	ITwibDebugger(uint32_t object_id, Twili &twili, trn::KDebug &&debug, std::shared_ptr<process::MonitoredProcess> proc);

	using CommandID = protocol::ITwibDebugger::Command;
	
 private:
	Twili &twili;
	trn::KDebug debug;
	std::shared_ptr<trn::WaitHandle> wait_handle;
	std::shared_ptr<process::MonitoredProcess> proc;
	
	void QueryMemory(bridge::ResponseOpener opener, uint64_t address);
	void ReadMemory(bridge::ResponseOpener opener, uint64_t address, uint64_t size);
	void WriteMemory(bridge::ResponseOpener opener, uint64_t address, InputStream &data);
	void ListThreads(bridge::ResponseOpener opener);
	void GetDebugEvent(bridge::ResponseOpener opener);
	void GetThreadContext(bridge::ResponseOpener opener, uint64_t thread_id);
	void BreakProcess(bridge::ResponseOpener opener);
	void ContinueDebugEvent(bridge::ResponseOpener opener, uint32_t flags, std::vector<uint64_t> thread_ids);
	void SetThreadContext(bridge::ResponseOpener opener, uint64_t thread_id, uint32_t flags, thread_context_t context);
	void GetNsoInfos(bridge::ResponseOpener opener);
	void WaitEvent(bridge::ResponseOpener opener);
	void GetTargetEntry(bridge::ResponseOpener opener);
	void LaunchDebugProcess(bridge::ResponseOpener opener);

 public:
	SmartRequestDispatcher<
		ITwibDebugger,
		SmartCommand<CommandID::QUERY_MEMORY, &ITwibDebugger::QueryMemory>,
		SmartCommand<CommandID::READ_MEMORY, &ITwibDebugger::ReadMemory>,
		SmartCommand<CommandID::WRITE_MEMORY, &ITwibDebugger::WriteMemory>,
		SmartCommand<CommandID::LIST_THREADS, &ITwibDebugger::ListThreads>,
		SmartCommand<CommandID::GET_DEBUG_EVENT, &ITwibDebugger::GetDebugEvent>,
		SmartCommand<CommandID::GET_THREAD_CONTEXT, &ITwibDebugger::GetThreadContext>,
		SmartCommand<CommandID::BREAK_PROCESS, &ITwibDebugger::BreakProcess>,
		SmartCommand<CommandID::CONTINUE_DEBUG_EVENT, &ITwibDebugger::ContinueDebugEvent>,
		SmartCommand<CommandID::SET_THREAD_CONTEXT, &ITwibDebugger::SetThreadContext>,
		SmartCommand<CommandID::GET_NSO_INFOS, &ITwibDebugger::GetNsoInfos>,
		SmartCommand<CommandID::WAIT_EVENT, &ITwibDebugger::WaitEvent>,
		SmartCommand<CommandID::GET_TARGET_ENTRY, &ITwibDebugger::GetTargetEntry>,
		SmartCommand<CommandID::LAUNCH_DEBUG_PROCESS, &ITwibDebugger::LaunchDebugProcess>
		> dispatcher;
};

} // namespace bridge
} // namespace twili
