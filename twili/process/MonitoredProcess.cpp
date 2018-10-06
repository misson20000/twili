#include "MonitoredProcess.hpp"

#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/util.h>

#include "Process.hpp"
#include "../twili.hpp"
#include "../ELFCrashReport.hpp"

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
