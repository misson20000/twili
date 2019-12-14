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

#include "ITwibDeviceInterface.hpp"

#include "Protocol.hpp"
#include "common/Logger.hpp"
#include "common/ResultError.hpp"
#include "err.hpp"

namespace twili {
namespace twib {
namespace tool {

ITwibDeviceInterface::ITwibDeviceInterface(std::shared_ptr<RemoteObject> obj) : obj(obj) {
}

ITwibProcessMonitor ITwibDeviceInterface::CreateMonitoredProcess(std::string type) {
	std::optional<ITwibProcessMonitor> mon;
	obj->SendSmartSyncRequest(
		CommandID::CREATE_MONITORED_PROCESS,
		in<std::string>(type),
		out_object<ITwibProcessMonitor>(mon));
	return *mon;
}

void ITwibDeviceInterface::Reboot() {
	obj->SendSmartSyncRequest(
		CommandID::REBOOT);
}

std::vector<uint8_t> ITwibDeviceInterface::CoreDump(uint64_t process_id) {
	std::vector<uint8_t> dump;
	obj->SendSmartSyncRequest(
		CommandID::COREDUMP,
		in<uint64_t>(process_id),
		out<std::vector<uint8_t>>(dump));
	return dump;
}

void ITwibDeviceInterface::Terminate(uint64_t process_id) {
	uint8_t *process_id_bytes = (uint8_t*) &process_id;
	obj->SendSmartSyncRequest(
		CommandID::TERMINATE,
		in<uint64_t>(process_id));
}

std::vector<ProcessListEntry> ITwibDeviceInterface::ListProcesses() {
	std::vector<ProcessListEntry> vec;
	obj->SendSmartSyncRequest(
		CommandID::LIST_PROCESSES,
		out<std::vector<ProcessListEntry>>(vec));
	return vec;
}

msgpack11::MsgPack ITwibDeviceInterface::Identify() {
	msgpack11::MsgPack ident;
	obj->SendSmartSyncRequest(
		CommandID::IDENTIFY,
		out(ident));
	return ident;
}

std::vector<std::string> ITwibDeviceInterface::ListNamedPipes() {
	std::vector<std::string> names;
	obj->SendSmartSyncRequest(
		CommandID::LIST_NAMED_PIPES,
		out(names));
	return names;
}

ITwibPipeReader ITwibDeviceInterface::OpenNamedPipe(std::string name) {
	std::optional<ITwibPipeReader> reader;
	obj->SendSmartSyncRequest(
		CommandID::OPEN_NAMED_PIPE,
		in(name),
		out_object(reader));
	return *reader;
}

ITwibDebugger ITwibDeviceInterface::OpenActiveDebugger(uint64_t pid) {
	std::optional<ITwibDebugger> debugger;
	obj->SendSmartSyncRequest(
		CommandID::OPEN_ACTIVE_DEBUGGER,
		in<uint64_t>(pid),
		out_object<ITwibDebugger>(debugger));
	return *debugger;
}

msgpack11::MsgPack ITwibDeviceInterface::GetMemoryInfo() {
	msgpack11::MsgPack pack;
	obj->SendSmartSyncRequest(
		CommandID::GET_MEMORY_INFO,
		out(pack));
	return pack;
}

void ITwibDeviceInterface::PrintDebugInfo() {
	obj->SendSmartSyncRequest(
		CommandID::PRINT_DEBUG_INFO);
}

uint64_t ITwibDeviceInterface::LaunchUnmonitoredProcess(uint64_t title_id, uint64_t storage_id, uint32_t launch_flags) {
	uint64_t pid;
	obj->SendSmartSyncRequest(
		CommandID::LAUNCH_UNMONITORED_PROCESS,
		in(launch_flags),
		in(title_id),
		in(storage_id),
		out(pid));
	return pid;
}

ITwibFilesystemAccessor ITwibDeviceInterface::OpenFilesystemAccessor(std::string fs) {
	std::optional<ITwibFilesystemAccessor> itfsa;
	obj->SendSmartSyncRequest(
		CommandID::OPEN_FILESYSTEM_ACCESSOR,
		in<std::string>(fs),
		out_object<ITwibFilesystemAccessor>(itfsa));
	return *itfsa;
}

void ITwibDeviceInterface::AsyncWaitToDebugApplication(std::function<void(uint32_t, uint64_t)> &&cb) {
	std::shared_ptr<uint64_t> pid_out = std::make_shared<uint64_t>(0);
	obj->SendSmartRequest(
		CommandID::WAIT_TO_DEBUG_APPLICATION,
		[cb{std::move(cb)}, pid_out](uint32_t r) {
			cb(r, *pid_out);
		},
		out<uint64_t>(*pid_out));
}

void ITwibDeviceInterface::AsyncWaitToDebugTitle(uint64_t tid, std::function<void(uint32_t, uint64_t)> &&cb) {
	std::shared_ptr<uint64_t> pid_out = std::make_shared<uint64_t>(0);
	obj->SendSmartRequest(
		CommandID::WAIT_TO_DEBUG_TITLE,
		[cb{std::move(cb)}, pid_out](uint32_t r) {
			cb(r, *pid_out);
		},
		in<uint64_t>(tid),
		out<uint64_t>(*pid_out));
}

uint64_t ITwibDeviceInterface::WaitToDebugApplication() {
	uint64_t pid;
	obj->SendSmartSyncRequest(
		CommandID::WAIT_TO_DEBUG_APPLICATION,
		out<uint64_t>(pid));
	return pid;
}

uint64_t ITwibDeviceInterface::WaitToDebugTitle(uint64_t tid) {
	uint64_t pid;
	obj->SendSmartSyncRequest(
		CommandID::WAIT_TO_DEBUG_TITLE,
		in<uint64_t>(tid),
		out<uint64_t>(pid));
	return pid;
}

void ITwibDeviceInterface::RebootUnsafe() {
	obj->SendSmartSyncRequest(
		CommandID::REBOOT_UNSAFE);
}

} // namespace tool
} // namespace twib
} // namespace twili
