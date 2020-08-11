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

#include "MonitoredProcess.hpp"

#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/util.h>

#include "Process.hpp"
#include "ProcessMonitor.hpp"
#include "../twili.hpp"
#include "../ELFCrashReport.hpp"
#include "../bridge/interfaces/ITwibPipeReader.hpp"
#include "../bridge/interfaces/ITwibPipeWriter.hpp"

#include "err.hpp"

namespace twili {
namespace process {

using trn::ResultCode;

MonitoredProcess::MonitoredProcess(Twili &twili) :
	Process(twili),
	tp_stdin(std::make_shared<TwibPipe>(twili.config.pipe_buffer_size_limit)),
	tp_stdout(std::make_shared<TwibPipe>(twili.config.pipe_buffer_size_limit)),
	tp_stderr(std::make_shared<TwibPipe>(twili.config.pipe_buffer_size_limit)) {

}

const char *MonitoredProcess::GetStateName(State state) {
	switch(state) {
	case State::Created: return "Created";
	case State::Started: return "Started";
	case State::Attached: return "Attached";
	case State::ShimSuspended: return "ShimSuspended";
	case State::Running: return "Running";
	case State::Crashed: return "Crashed";
	case State::Exited: return "Exited";
	default: return "Unknown";
	}
}

void MonitoredProcess::LaunchSuspended(bridge::ResponseOpener response) {
	suspended_launch_enabled = true;
	Launch(response);
}

uint64_t MonitoredProcess::GetPid() {
	if(!proc) {
		return 0;
	}
	return twili::Assert(trn::svc::GetProcessId(proc->handle));
}

void MonitoredProcess::AddNotes(ELFCrashReport &report) {
	Process::AddNotes(report);
	std::vector<ELF::Note::elf_auxv_t> auxv;
	auxv.push_back({ELF::AT_ENTRY, target_entry});
	auxv.push_back({ELF::AT_NULL, 0});
	report.AddNote("CORE", ELF::NT_AUXV, auxv);
}

void MonitoredProcess::Terminate() {
	if(state == State::Created) {
		ChangeState(State::Exited);
		return;
	}
	if(state == State::ShimSuspended) {
		// need to return from IPC first before process will actually die
		auto slcb = suspended_launch_cb;
		suspended_launch_cb = std::function<void(trn::ResultCode)>();
		slcb(TWILI_ERR_NO_LONGER_REQUESTED_TO_LAUNCH);
	}
	if(state == State::Exited) {
		return; // already dead
	}
	if(!proc) {
		return; // silently fail...
	}
	twili::Assert(trn::svc::TerminateProcess(*this->proc));
}

void MonitoredProcess::Kill() {
	if(state == State::Created) {
		ChangeState(State::Exited);
		return;
	}
	if(state == State::ShimSuspended) {
		auto slcb = suspended_launch_cb;
		suspended_launch_cb = std::function<void(trn::ResultCode)>();
		slcb(TWILI_ERR_NO_LONGER_REQUESTED_TO_LAUNCH);
		return; // let this happen cleanly
	}
	if(state == State::Exited) {
		return; // already dead
	}
	Terminate();
}

void MonitoredProcess::Continue() {
	if(state == State::ShimSuspended) {
		ChangeState(State::Running);
		
		auto slcb = suspended_launch_cb;
		suspended_launch_cb = std::function<void(trn::ResultCode)>();
		slcb(RESULT_OK);
	} else {
		// don't try to pause us later
		suspended_launch_enabled = false;
	}
}

void MonitoredProcess::PrintDebugInfo(const char *indent) {
	char sub_indent[64];
	snprintf(sub_indent, 64, "%s  ", indent);
	printf("%stp_stdin:\n", indent);
	tp_stdin->PrintDebugInfo(sub_indent);
	printf("%stp_stdout:\n", indent);
	tp_stdout->PrintDebugInfo(sub_indent);
	printf("%stp_stderr:\n", indent);
	tp_stderr->PrintDebugInfo(sub_indent);
}

MonitoredProcess::State MonitoredProcess::GetState() {
	return state;
}

trn::ResultCode MonitoredProcess::GetResult() {
	if(result.code != 0) {
		return result;
	} else {
		return TWILI_ERR_UNSPECIFIED;
	}
}

void MonitoredProcess::AppendCode(std::shared_ptr<fs::ProcessFile> file) {
	files.push_back(file);
}

void MonitoredProcess::Attach(std::shared_ptr<trn::KProcess> process) {
	if(proc) {
		// logic error
		twili::Abort(TWILI_ERR_MONITORED_PROCESS_ALREADY_ATTACHED);
	}
	proc = process;
	ChangeState(State::Attached);
}

void MonitoredProcess::SetResult(trn::ResultCode r) {
	result = r;
}

void MonitoredProcess::ChangeState(State new_state) {
	printf("process [0x%lx] changing to state %s\n", GetPid(), GetStateName(new_state));
	if(new_state == State::Exited) {
		// close pipes
		tp_stdin->CloseReader();
		tp_stdout->CloseWriter();
		tp_stderr->CloseWriter();

		if(suspended_launch_cb) {
			auto slcb = suspended_launch_cb;
			suspended_launch_cb = std::function<void(trn::ResultCode)>();
			slcb(TWILI_ERR_NO_LONGER_REQUESTED_TO_LAUNCH);
		}
	}
	state = new_state;
	const std::list<ProcessMonitor*> monitors_immut(monitors);
	for(auto mon : monitors_immut) {
		mon->StateChanged(new_state);
	}
}

std::shared_ptr<trn::KProcess> MonitoredProcess::GetProcess() {
	return proc;
}

uint64_t MonitoredProcess::GetTargetEntry() {
	return target_entry;
}

void MonitoredProcess::AddHBABIEntries(std::vector<loader_config_entry_t> &entries) {
	entries.push_back({
		.key = LCONFIG_KEY_TWILI_PRESENT,
		.flags = 0,
	});
	entries.push_back({
		.key = LCONFIG_KEY_SYSCALL_AVAILABLE_HINT,
		.flags = 0,
		.syscall_available_hint = {
			0xffffffffffffffff,
			0xffffffffffffffff
		}
	});
}

void MonitoredProcess::WaitToStart(std::function<void(trn::ResultCode)> cb) {
	if(suspended_launch_enabled) {
		ChangeState(State::ShimSuspended);
		suspended_launch_cb = cb;
	} else {
		ChangeState(State::Running);
		cb(RESULT_OK);
	}
}

void MonitoredProcess::AddMonitor(ProcessMonitor &monitor) {
	monitors.push_back(&monitor);
}

void MonitoredProcess::RemoveMonitor(ProcessMonitor &monitor) {
	monitors.remove(&monitor);
}

MonitoredProcess::~MonitoredProcess() {
	tp_stdin->CloseReader();
	tp_stdout->CloseWriter();
	tp_stderr->CloseWriter();
}

} // namespace process
} // namespace twili
