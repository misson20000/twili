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

#include<deque>
#include<optional>

#include "../Object.hpp"
#include "../ResponseOpener.hpp"
#include "../../process/ProcessMonitor.hpp"
#include "../../process/MonitoredProcess.hpp"

namespace twili {

class Twili;

namespace bridge {

class ITwibProcessMonitor : public bridge::Object, public process::ProcessMonitor {
 public:
	ITwibProcessMonitor(uint32_t object_id, std::shared_ptr<process::MonitoredProcess> process);
	virtual ~ITwibProcessMonitor() override;
	
	virtual void HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, bridge::ResponseOpener opener);

	virtual void StateChanged(process::MonitoredProcess::State new_state) override;
 private:
	void Launch(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void Terminate(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void AppendCode(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	
	void OpenStdin(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void OpenStdout(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void OpenStderr(std::vector<uint8_t> payload, bridge::ResponseOpener opener);

	void WaitStateChange(std::vector<uint8_t> payload, bridge::ResponseOpener opener);

	std::deque<process::MonitoredProcess::State> state_changes;
	std::optional<bridge::ResponseOpener> state_observer;
};

} // namespace bridge
} // namespace twili
