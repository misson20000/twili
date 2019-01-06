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

	using CommandID = protocol::ITwibProcessMonitor::Command;
	
	virtual RequestHandler *OpenRequest(uint32_t command_id, size_t payload_size, bridge::ResponseOpener opener) override;

	virtual void StateChanged(process::MonitoredProcess::State new_state) override;
 private:
	void Launch(bridge::ResponseOpener opener);
	void Terminate(bridge::ResponseOpener opener);
	void AppendCode(bridge::ResponseOpener opener, InputStream &code);
	
	void OpenStdin(bridge::ResponseOpener opener);
	void OpenStdout(bridge::ResponseOpener opener);
	void OpenStderr(bridge::ResponseOpener opener);

	void WaitStateChange(bridge::ResponseOpener opener);

	SmartRequestDispatcher<
		ITwibProcessMonitor,
		SmartCommand<CommandID::LAUNCH, &ITwibProcessMonitor::Launch>,
		SmartCommand<CommandID::TERMINATE, &ITwibProcessMonitor::Terminate>,
		SmartCommand<CommandID::APPEND_CODE, &ITwibProcessMonitor::AppendCode>,
		SmartCommand<CommandID::OPEN_STDIN, &ITwibProcessMonitor::OpenStdin>,
		SmartCommand<CommandID::OPEN_STDOUT, &ITwibProcessMonitor::OpenStdout>,
		SmartCommand<CommandID::OPEN_STDERR, &ITwibProcessMonitor::OpenStderr>,
		SmartCommand<CommandID::WAIT_STATE_CHANGE, &ITwibProcessMonitor::WaitStateChange>
		> dispatcher;
	
	std::deque<process::MonitoredProcess::State> state_changes;
	std::optional<bridge::ResponseOpener> state_observer;
};

} // namespace bridge
} // namespace twili
