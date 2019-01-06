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

#include "ITwibProcessMonitor.hpp"

#include<libtransistor/cpp/nx.hpp>

#include "../../twili.hpp"
#include "../../process/MonitoredProcess.hpp"

#include "ITwibPipeReader.hpp"
#include "ITwibPipeWriter.hpp"

#include "err.hpp"

using namespace trn;

namespace twili {
namespace bridge {

ITwibProcessMonitor::ITwibProcessMonitor(uint32_t object_id, std::shared_ptr<process::MonitoredProcess> process) : Object(object_id), process::ProcessMonitor(process), dispatcher(*this) {
}

ITwibProcessMonitor::~ITwibProcessMonitor() {
	process->Kill(); // attempt to exit cleanly
}

RequestHandler *ITwibProcessMonitor::OpenRequest(uint32_t command_id, size_t payload_size, bridge::ResponseOpener opener) {
	return dispatcher.SmartDispatch(command_id, payload_size, opener);
}

void ITwibProcessMonitor::StateChanged(process::MonitoredProcess::State new_state) {
	if(state_observer) {
		auto w = state_observer->BeginOk(sizeof(uint32_t));
		w.Write<uint32_t>((uint32_t) new_state);
		w.Finalize();
		state_observer.reset();
	} else {
		state_changes.push_back(new_state);
	}
}

void ITwibProcessMonitor::Launch(bridge::ResponseOpener opener) {
	process->twili.monitored_processes.push_back(process);
	printf("  began monitoring 0x%x\n", process->GetPid());

	process->Launch(opener);
}

void ITwibProcessMonitor::Terminate(bridge::ResponseOpener opener) {
	process->Terminate();
	opener.BeginOk().Finalize();
}

void ITwibProcessMonitor::AppendCode(bridge::ResponseOpener opener, std::vector<uint8_t> code) {
	process->AppendCode(code);
	opener.BeginOk().Finalize();
}

void ITwibProcessMonitor::OpenStdin(bridge::ResponseOpener opener) {
	opener.RespondOk(opener.MakeObject<ITwibPipeWriter>(process->tp_stdin));
}

void ITwibProcessMonitor::OpenStdout(bridge::ResponseOpener opener) {
	opener.RespondOk(opener.MakeObject<ITwibPipeReader>(process->tp_stdout));
}

void ITwibProcessMonitor::OpenStderr(bridge::ResponseOpener opener) {
	opener.RespondOk(opener.MakeObject<ITwibPipeReader>(process->tp_stderr));
}

void ITwibProcessMonitor::WaitStateChange(bridge::ResponseOpener opener) {
	if(state_changes.size() > 0) {
		opener.RespondOk((uint32_t) state_changes.front());
		state_changes.pop_front();
	} else {
		if(state_observer) {
			state_observer->RespondError(TWILI_ERR_INTERRUPTED);
		}
		state_observer = opener;
	}
}

} // namespace bridge
} // namespace twili
