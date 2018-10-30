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

namespace twili {

class Twili;

namespace bridge {

class ITwibDeviceInterface : public bridge::Object {
 public:
	ITwibDeviceInterface(uint32_t object_id, Twili &twili);
	virtual void HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, bridge::ResponseOpener opener);
 private:
	Twili &twili;
	
	void CreateMonitoredProcess(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void Reboot(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void CoreDump(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void Terminate(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void ListProcesses(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void UpgradeTwili(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void Identify(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void ListNamedPipes(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void OpenNamedPipe(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void OpenActiveDebugger(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void GetMemoryInfo(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
};

} // namespace bridge
} // namespace twili
