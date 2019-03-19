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

#include "../Object.hpp"
#include "../ResponseOpener.hpp"
#include "../RequestHandler.hpp"

namespace twili {

class Twili;

namespace bridge {

class ITwibDeviceInterface : public ObjectDispatcherProxy<ITwibDeviceInterface> {
 public:
	ITwibDeviceInterface(uint32_t object_id, Twili &twili);

	using CommandID = protocol::ITwibDeviceInterface::Command;
	
 private:
	Twili &twili;
	
	void CreateMonitoredProcess(bridge::ResponseOpener opener, std::string type);
	void Reboot(bridge::ResponseOpener opener);
	void CoreDump(bridge::ResponseOpener opener, uint64_t pid);
	void Terminate(bridge::ResponseOpener opener, uint64_t pid);
	void ListProcesses(bridge::ResponseOpener opener);
	void UpgradeTwili(bridge::ResponseOpener opener);
	void Identify(bridge::ResponseOpener opener);
	void ListNamedPipes(bridge::ResponseOpener opener);
	void OpenNamedPipe(bridge::ResponseOpener opener, std::string name);
	void OpenActiveDebugger(bridge::ResponseOpener opener, uint64_t pid);
	void GetMemoryInfo(bridge::ResponseOpener opener);
	void PrintDebugInfo(bridge::ResponseOpener opener);
	void LaunchUnmonitoredProcess(bridge::ResponseOpener opener, uint32_t flags, uint64_t tid, uint64_t storage);
	void OpenFilesystemAccessor(bridge::ResponseOpener opener, std::string fs);
	void WaitToDebugApplication(bridge::ResponseOpener opener);
	void WaitToDebugTitle(bridge::ResponseOpener opener, uint64_t tid);

 public:
	SmartRequestDispatcher<
		ITwibDeviceInterface,
		SmartCommand<CommandID::CREATE_MONITORED_PROCESS, &ITwibDeviceInterface::CreateMonitoredProcess>,
		SmartCommand<CommandID::REBOOT, &ITwibDeviceInterface::Reboot>,
		SmartCommand<CommandID::COREDUMP, &ITwibDeviceInterface::CoreDump>,
		SmartCommand<CommandID::TERMINATE, &ITwibDeviceInterface::Terminate>,
		SmartCommand<CommandID::LIST_PROCESSES, &ITwibDeviceInterface::ListProcesses>,
		SmartCommand<CommandID::UPGRADE_TWILI, &ITwibDeviceInterface::UpgradeTwili>,
		SmartCommand<CommandID::IDENTIFY, &ITwibDeviceInterface::Identify>,
		SmartCommand<CommandID::LIST_NAMED_PIPES, &ITwibDeviceInterface::ListNamedPipes>,
		SmartCommand<CommandID::OPEN_NAMED_PIPE, &ITwibDeviceInterface::OpenNamedPipe>,
		SmartCommand<CommandID::OPEN_ACTIVE_DEBUGGER, &ITwibDeviceInterface::OpenActiveDebugger>,
		SmartCommand<CommandID::GET_MEMORY_INFO, &ITwibDeviceInterface::GetMemoryInfo>,
		SmartCommand<CommandID::PRINT_DEBUG_INFO, &ITwibDeviceInterface::PrintDebugInfo>,
		SmartCommand<CommandID::LAUNCH_UNMONITORED_PROCESS, &ITwibDeviceInterface::LaunchUnmonitoredProcess>,
		SmartCommand<CommandID::OPEN_FILESYSTEM_ACCESSOR, &ITwibDeviceInterface::OpenFilesystemAccessor>,
		SmartCommand<CommandID::WAIT_TO_DEBUG_APPLICATION, &ITwibDeviceInterface::WaitToDebugApplication>,
		SmartCommand<CommandID::WAIT_TO_DEBUG_TITLE, &ITwibDeviceInterface::WaitToDebugTitle>
		> dispatcher;

	trn::KEvent ev_debug_application;
	trn::KEvent ev_debug_title;
	std::shared_ptr<trn::WaitHandle> wh_debug_application;
	std::shared_ptr<trn::WaitHandle> wh_debug_title;
};

} // namespace bridge
} // namespace twili
