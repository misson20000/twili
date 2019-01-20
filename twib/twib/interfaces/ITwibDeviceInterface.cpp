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
#include "Logger.hpp"
#include "ResultError.hpp"
#include "err.hpp"

namespace twili {
namespace twib {

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

} // namespace twib
} // namespace twili
