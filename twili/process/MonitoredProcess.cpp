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
	if(!proc) {
		return; // silently fail...
	}
	ResultCode::AssertOk(trn::svc::TerminateProcess(*this->proc));
}

void MonitoredProcess::Kill() {
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
