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

ITwibProcessMonitor::ITwibProcessMonitor(uint32_t object_id, std::shared_ptr<process::MonitoredProcess> process) : Object(object_id), process::ProcessMonitor(process) {
}

ITwibProcessMonitor::~ITwibProcessMonitor() {
	process->Kill(); // attempt to exit cleanly
}

void ITwibProcessMonitor::HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	switch((protocol::ITwibProcessMonitor::Command) command_id) {
	case protocol::ITwibProcessMonitor::Command::LAUNCH:
		Launch(payload, opener);
		break;
	case protocol::ITwibProcessMonitor::Command::TERMINATE:
		Terminate(payload, opener);
		break;
	case protocol::ITwibProcessMonitor::Command::APPEND_CODE:
		AppendCode(payload, opener);
		break;
	case protocol::ITwibProcessMonitor::Command::OPEN_STDIN:
		OpenStdin(payload, opener);
		break;
	case protocol::ITwibProcessMonitor::Command::OPEN_STDOUT:
		OpenStdout(payload, opener);
		break;
	case protocol::ITwibProcessMonitor::Command::OPEN_STDERR:
		OpenStderr(payload, opener);
		break;
	case protocol::ITwibProcessMonitor::Command::WAIT_STATE_CHANGE:
		WaitStateChange(payload, opener);
		break;
	default:
		opener.BeginError(ResultCode(TWILI_ERR_PROTOCOL_UNRECOGNIZED_FUNCTION)).Finalize();
		break;
	}
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

void ITwibProcessMonitor::Launch(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() > 0) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}

	process->twili.monitored_processes.push_back(process);
	printf("  began monitoring 0x%x\n", process->GetPid());

	process->Launch(opener);
}

void ITwibProcessMonitor::Terminate(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() > 0) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}

	process->Terminate();
	opener.BeginOk().Finalize();
}

void ITwibProcessMonitor::AppendCode(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	process->AppendCode(payload);
	opener.BeginOk().Finalize();
}

void ITwibProcessMonitor::OpenStdin(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() > 0) { throw ResultError(TWILI_ERR_BAD_REQUEST); }
	auto w = opener.BeginOk(sizeof(uint32_t), 1);
	w.Write<uint32_t>(w.Object(opener.MakeObject<ITwibPipeWriter>(process->tp_stdin)));
	w.Finalize();
}

void ITwibProcessMonitor::OpenStdout(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() > 0) { throw ResultError(TWILI_ERR_BAD_REQUEST); }
	auto w = opener.BeginOk(sizeof(uint32_t), 1);
	w.Write<uint32_t>(w.Object(opener.MakeObject<ITwibPipeReader>(process->tp_stdout)));
	w.Finalize();
}

void ITwibProcessMonitor::OpenStderr(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() > 0) { throw ResultError(TWILI_ERR_BAD_REQUEST); }
	auto w = opener.BeginOk(sizeof(uint32_t), 1);
	w.Write<uint32_t>(w.Object(opener.MakeObject<ITwibPipeReader>(process->tp_stderr)));
	w.Finalize();
}

void ITwibProcessMonitor::WaitStateChange(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() > 0) { throw ResultError(TWILI_ERR_BAD_REQUEST); }
	if(state_changes.size() > 0) {
		auto w = opener.BeginOk(sizeof(uint32_t));
		w.Write<uint32_t>((uint32_t) state_changes.front());
		w.Finalize();
		state_changes.pop_front();
	} else {
		if(state_observer) {
			state_observer->BeginError(TWILI_ERR_INTERRUPTED).Finalize();
		}
		state_observer = opener;
	}
}

} // namespace bridge
} // namespace twili
