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

MonitoredProcess::MonitoredProcess(Twili &twili) : Process(twili) {

}

uint64_t MonitoredProcess::GetPid() {
	if(!proc) {
		return 0;
	}
	return ResultCode::AssertOk(trn::svc::GetProcessId(proc->handle));
}

void MonitoredProcess::AddNotes(ELFCrashReport &report) {
	Process::AddNotes(report);
	std::vector<ELF::Note::elf_auxv_t> auxv;
	auxv.push_back({ELF::AT_ENTRY, target_entry});
	auxv.push_back({ELF::AT_NULL, 0});
	report.AddNote("CORE", ELF::NT_AUXV, auxv);
}

void MonitoredProcess::Terminate() {
	if(state == State::Exited) {
		return; // already dead
	}
	if(!proc) {
		return; // silently fail...
	}
	ResultCode::AssertOk(trn::svc::TerminateProcess(*this->proc));
}

void MonitoredProcess::Kill() {
	if(state == State::Exited) {
		return; // already dead
	}
	Terminate();
}

MonitoredProcess::State MonitoredProcess::GetState() {
	return state;
}

trn::ResultCode MonitoredProcess::GetResult() {
	return result;
}

void MonitoredProcess::Attach(std::shared_ptr<trn::KProcess> process) {
	if(proc) {
		throw trn::ResultError(TWILI_ERR_MONITORED_PROCESS_ALREADY_ATTACHED);
	}
	proc = process;
	ChangeState(State::Attached);
}

void MonitoredProcess::SetResult(trn::ResultCode r) {
	result = r;
}

void MonitoredProcess::ChangeState(State new_state) {
	printf("process [0x%lx] changing to state %d\n", GetPid(), (int) new_state);
	if(new_state == State::Running) {
		twili.monitored_processes.push_back(shared_from_this());
		printf("  began monitoring 0x%x\n", GetPid());
	}
	if(new_state == State::Exited) {
		// close pipes
		tp_stdin->Close();
		tp_stdout->Close();
		tp_stderr->Close();
	}
	state = new_state;
	for(auto mon : monitors) {
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

void MonitoredProcess::AddMonitor(ProcessMonitor &monitor) {
	monitors.push_back(&monitor);
}

void MonitoredProcess::RemoveMonitor(ProcessMonitor &monitor) {
	monitors.remove(&monitor);
}

MonitoredProcess::~MonitoredProcess() {
	tp_stdin->Close();
	tp_stdout->Close();
	tp_stderr->Close();
}

} // namespace process
} // namespace twili
