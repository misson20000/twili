#include "MonitoredProcess.hpp"

#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/util.h>

#include "Process.hpp"
#include "ELFCrashReport.hpp"
#include "twili.hpp"

namespace twili {

using trn::ResultCode;

MonitoredProcess::MonitoredProcess(Twili *twili, std::shared_ptr<trn::KProcess> proc, uint64_t target_entry) :
	twili(twili),
	proc(proc),
	target_entry(target_entry),
	Process(ResultCode::AssertOk(trn::svc::GetProcessId(proc->handle))) {

	printf("created monitored process: 0x%x, pid 0x%x\n", proc->handle, pid);
	wait = twili->event_waiter.Add(*proc, [this]() {
			printf("monitored process (0x%x) signalled\n", this->proc->handle);
			this->proc->ResetSignal();
			auto state = (trn::svc::ProcessState) ResultCode::AssertOk(trn::svc::GetProcessInfo(*this->proc, 0));
			printf("  state: 0x%lx\n", state);
			if(state == trn::svc::ProcessState::CRASHED) {
				printf("monitored process (0x%x) crashed\n", this->proc->handle);
				printf("ready to generate crash report\n");
				crashed = true;
			}
			if(state == trn::svc::ProcessState::EXITED) {
				printf("monitored process (0x%x) exited\n", this->proc->handle);
				destroy_flag = true;
			}
			return true;
		});
}

void MonitoredProcess::Launch() {
	printf("launching monitored process: 0x%x\n", proc->handle);
	trn::ResultCode::AssertOk(
		trn::svc::StartProcess(*proc, 58, 0, 0x100000));
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

trn::Result<std::nullopt_t> MonitoredProcess::GenerateCrashReport(ELFCrashReport &report, usb::USBBridge::ResponseOpener opener) {
	printf("generating crash report...\n");

	std::vector<ELF::Note::elf_auxv_t> auxv;
	auxv.push_back({ELF::AT_ENTRY, target_entry});
	auxv.push_back({ELF::AT_NULL, 0});
	report.AddNote("CORE", ELF::NT_AUXV, auxv);
	
	auto r = Process::GenerateCrashReport(report, opener);
	Terminate();
	return r;
}

trn::Result<std::nullopt_t> MonitoredProcess::Terminate() {
   return trn::svc::TerminateProcess(*this->proc);
}

MonitoredProcess::~MonitoredProcess() {
}

}
