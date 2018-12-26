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
	Response rs = obj->SendSyncRequest(protocol::ITwibDeviceInterface::Command::CREATE_MONITORED_PROCESS, std::vector<uint8_t>(type.begin(), type.end()));
	return ITwibProcessMonitor(rs.objects[0]);
}

void ITwibDeviceInterface::Reboot() {
	obj->SendSyncRequest(protocol::ITwibDeviceInterface::Command::REBOOT);
}

std::vector<uint8_t> ITwibDeviceInterface::CoreDump(uint64_t process_id) {
    uint8_t *process_id_bytes = (uint8_t*) &process_id;
    return obj->SendSyncRequest(protocol::ITwibDeviceInterface::Command::COREDUMP, std::vector<uint8_t>(process_id_bytes, process_id_bytes + sizeof(process_id))).payload;
}

void ITwibDeviceInterface::Terminate(uint64_t process_id) {
	uint8_t *process_id_bytes = (uint8_t*) &process_id;
	obj->SendSyncRequest(protocol::ITwibDeviceInterface::Command::TERMINATE, std::vector<uint8_t>(process_id_bytes, process_id_bytes + sizeof(process_id)));
}

std::vector<ProcessListEntry> ITwibDeviceInterface::ListProcesses() {
	Response rs = obj->SendSyncRequest(protocol::ITwibDeviceInterface::Command::LIST_PROCESSES);
	ProcessListEntry *first = (ProcessListEntry*) rs.payload.data();
	return std::vector<ProcessListEntry>(first, first + (rs.payload.size() / sizeof(ProcessListEntry)));
}

msgpack11::MsgPack ITwibDeviceInterface::Identify() {
	Response rs = obj->SendSyncRequest(protocol::ITwibDeviceInterface::Command::IDENTIFY);
	std::string err;
	return msgpack11::MsgPack::parse(std::string(rs.payload.begin(), rs.payload.end()), err);
}

std::vector<std::string> ITwibDeviceInterface::ListNamedPipes() {
	Response rs = obj->SendSyncRequest(protocol::ITwibDeviceInterface::Command::LIST_NAMED_PIPES);
	uint32_t count = *(uint32_t*) rs.payload.data();
	std::vector<std::string> names;
	
	size_t pos = 4;
	for(uint32_t i = 0; i < count; i++) {
		uint32_t size = *(uint32_t*) (rs.payload.data() + pos);
		pos+= 4;
		names.emplace_back(rs.payload.data() + pos, rs.payload.data() + pos + size);
		pos+= size;
	}

	return names;
}

ITwibPipeReader ITwibDeviceInterface::OpenNamedPipe(std::string name) {
	Response rs = obj->SendSyncRequest(protocol::ITwibDeviceInterface::Command::OPEN_NAMED_PIPE, std::vector<uint8_t>(name.begin(), name.end()));
	if(rs.payload.size() < sizeof(uint32_t)) {
		throw ResultError(TWILI_ERR_BAD_RESPONSE);
	}
	return rs.objects[*(uint32_t*) rs.payload.data()];
}

ITwibDebugger ITwibDeviceInterface::OpenActiveDebugger(uint64_t pid) {
	uint8_t *pid_bytes = (uint8_t*) &pid;
	Response rs = obj->SendSyncRequest(protocol::ITwibDeviceInterface::Command::OPEN_ACTIVE_DEBUGGER, std::vector<uint8_t>(pid_bytes, pid_bytes + sizeof(pid)));
	if(rs.payload.size() < sizeof(uint32_t)) {
		throw ResultError(TWILI_ERR_BAD_RESPONSE);
	}
	return rs.objects[*(uint32_t*) rs.payload.data()];
}

msgpack11::MsgPack ITwibDeviceInterface::GetMemoryInfo() {
	Response rs = obj->SendSyncRequest(protocol::ITwibDeviceInterface::Command::GET_MEMORY_INFO);
	std::string err;
	return msgpack11::MsgPack::parse(std::string(rs.payload.begin(), rs.payload.end()), err);
}

void ITwibDeviceInterface::PrintDebugInfo() {
	obj->SendSyncRequest(protocol::ITwibDeviceInterface::Command::PRINT_DEBUG_INFO);
}

} // namespace twib
} // namespace twili
