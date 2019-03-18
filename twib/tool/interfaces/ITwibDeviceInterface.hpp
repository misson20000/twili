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

#include<vector>

#include<msgpack11.hpp>

#include "../RemoteObject.hpp"
#include "ITwibPipeWriter.hpp"
#include "ITwibPipeReader.hpp"
#include "ITwibProcessMonitor.hpp"
#include "ITwibFilesystemAccessor.hpp"

namespace twili {
namespace twib {
namespace tool {

struct ProcessListEntry {
	uint64_t process_id;
	uint32_t result;
	uint64_t title_id;
	char process_name[12];
	uint32_t mmu_flags;
};

class ITwibDeviceInterface {
 public:
	ITwibDeviceInterface(std::shared_ptr<RemoteObject> obj);

	using CommandID = protocol::ITwibDeviceInterface::Command;
	
	ITwibProcessMonitor CreateMonitoredProcess(std::string type);
	void Reboot();
	std::vector<uint8_t> CoreDump(uint64_t process_id);
	void Terminate(uint64_t process_id);
	std::vector<ProcessListEntry> ListProcesses();
	msgpack11::MsgPack Identify();
	std::vector<std::string> ListNamedPipes();
	ITwibPipeReader OpenNamedPipe(std::string name);
	msgpack11::MsgPack GetMemoryInfo();
	void PrintDebugInfo();
	uint64_t LaunchUnmonitoredProcess(uint64_t title_id, uint64_t storage_id, uint32_t launch_flags);
	ITwibFilesystemAccessor OpenFilesystemAccessor(std::string name);
 private:
	std::shared_ptr<RemoteObject> obj;
};

} // namespace tool
} // namespace twib
} // namespace twili
