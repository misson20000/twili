#include "MonitoredProcess.hpp"

#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/util.h>

#include "Process.hpp"
#include "../twili.hpp"
#include "../ELFCrashReport.hpp"
#include "../bridge/interfaces/ITwibPipeReader.hpp"
#include "../bridge/interfaces/ITwibPipeWriter.hpp"

#include "err.hpp"

namespace twili {
namespace process {

using trn::ResultCode;

MonitoredProcess::MonitoredProcess(Twili &twili, bridge::ResponseOpener attachment_opener) : Process(twili), attachment_opener(attachment_opener) {

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
}

void MonitoredProcess::SetResult(trn::ResultCode r) {
	result = r;
}

void MonitoredProcess::ChangeState(State new_state) {
	if(new_state == State::Exited) {
		if(state == State::Starting) {
			if(attachment_opener) {
				attachment_opener->BeginError(result).Finalize();
				attachment_opener.reset();
			}
		}
	}
	if(new_state == State::Running) {
		struct {
			uint64_t pid;
			uint32_t tp_stdin;
			uint32_t tp_stdout;
			uint32_t tp_stderr;
		} response;

		auto w = attachment_opener->BeginOk(sizeof(response), 3);
	
		response.pid = GetPid();
		response.tp_stdin  = w.Object(attachment_opener->MakeObject<bridge::ITwibPipeWriter>(tp_stdin ));
		response.tp_stdout = w.Object(attachment_opener->MakeObject<bridge::ITwibPipeReader>(tp_stdout));
		response.tp_stderr = w.Object(attachment_opener->MakeObject<bridge::ITwibPipeReader>(tp_stderr));
	
		w.Write<decltype(response)>(response);
	
		w.Finalize();

		attachment_opener.reset();
	
		twili.monitored_processes.push_back(shared_from_this());
		printf("  began monitoring 0x%x\n", GetPid());
	}
	
	state = new_state;
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

MonitoredProcess::~MonitoredProcess() {
	tp_stdin->Close();
	tp_stdout->Close();
	tp_stderr->Close();
}

} // namespace process
} // namespace twili
