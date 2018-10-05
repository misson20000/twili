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

class CrashReportException {
 public:
	CrashReportException(debug_event_info_t &info) :
		info(info) {
	}

	debug_event_info_t info;
};

class CrashReportThread {
 public:
	CrashReportThread(debug_event_info_t &info) :
		thread_id(info.attach_thread.thread_id),
		tls_pointer(info.attach_thread.tls_pointer),
		entrypoint(info.attach_thread.entrypoint) {
		
	}

	uint64_t thread_id;
	uint64_t tls_pointer;
	uint64_t entrypoint;
};

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

MonitoredProcess::~MonitoredProcess() {
	tp_stdin->Close();
	tp_stdout->Close();
	tp_stderr->Close();
}

} // namespace process
} // namespace twili
