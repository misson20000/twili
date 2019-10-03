//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2019 misson20000 <xenotoad@xenotoad.net>
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

#include "AppletProcess.hpp"

#include "../twili.hpp"
#include "fs/ActualFile.hpp"
#include "fs/VectorFile.hpp"

#include "err.hpp"
#include "title_id.hpp"

using namespace trn;

namespace twili {
namespace process {

AppletProcess::AppletProcess(AppletTracker &tracker) :
	TrackedProcess<AppletProcess>(
		tracker.twili,
		tracker,
		"/squash/applet_host.nso",
		"/squash/applet_host.npdm",
		title_id::AppletShimTitle),
	command_wevent(command_event) {
}

void AppletProcess::ChangeState(State state) {
	TrackedProcess<AppletProcess>::ChangeState(state);
	if(state == State::Exited) {
		kill_timeout.reset();
	}
}

void AppletProcess::KillImpl() {
	std::shared_ptr<MonitoredProcess> self = shared_from_this();

	// if we don't exit within 10 seconds, go ahead and terminate
	kill_timeout = twili.event_waiter.AddDeadline(
		svcGetSystemTick() + 10000000000uLL,
		[self, this]() -> uint64_t {
			printf("AppletProcess: clean exit timeout expired, terminating...\n");
			self->Terminate();
			kill_timeout.reset(); // make sure self ptr gets cleaned up
			return 0; // don't rearm
		});

	printf("AppletProcess: requesting to exit cleanly...\n");
	PushCommand(0); // REQUEST_EXIT
}

void AppletProcess::AddHBABIEntries(std::vector<loader_config_entry_t> &entries) {
	MonitoredProcess::AddHBABIEntries(entries);
	entries.push_back({
			.key = LCONFIG_KEY_APPLET_TYPE,
			.flags = 0,
			.applet_type = {
				.applet_type = LCONFIG_APPLET_TYPE_LIBRARY_APPLET,
			}
		});
}

trn::KEvent &AppletProcess::GetCommandEvent() {
	return command_event;
}

std::optional<uint32_t> AppletProcess::PopCommand() {
	if(commands.size() == 0) {
		return std::nullopt;
	} else {
		uint32_t cmd = commands.front();
		commands.pop_front();
		return cmd;
	}
}

void AppletProcess::PushCommand(uint32_t command) {
	commands.push_back(command);
	command_wevent.Signal();
}

} // namespace process
} // namespace twili
