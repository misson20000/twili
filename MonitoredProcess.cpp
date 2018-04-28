#include<libtransistor/cpp/svc.hpp>

#include "MonitoredProcess.hpp"
#include "twili.hpp"

namespace twili {

using Transistor::ResultCode;

MonitoredProcess::MonitoredProcess(Twili *twili, std::shared_ptr<Transistor::KProcess> proc) :
	twili(twili),
	proc(proc) {

	printf("created monitored process: 0x%x\n", proc->handle);
	wait = twili->event_waiter.Add(*proc, [this]() {
			printf("monitored process (0x%x) signalled\n", this->proc->handle);
			this->proc->ResetSignal();
			auto state = (Transistor::SVC::ProcessState) ResultCode::AssertOk(Transistor::SVC::GetProcessInfo(*this->proc, 0));
			printf("  state: 0x%lx\n", state);
			if(state == Transistor::SVC::ProcessState::CRASHED) {
				printf("monitored process (0x%x) crashed\n", this->proc->handle);
				GenerateCrashReport();
				Transistor::SVC::TerminateProcess(*this->proc);
			}
			if(state == Transistor::SVC::ProcessState::EXITED) {
				printf("monitored process (0x%x) exited\n", this->proc->handle);
				destroy_flag = true;
			}
			return true;
		});
}

void MonitoredProcess::Launch() {
	printf("launching monitored process: 0x%x\n", proc->handle);
	Transistor::ResultCode::AssertOk(
		Transistor::SVC::StartProcess(*proc, 58, 0, 0x100000));
}

void MonitoredProcess::GenerateCrashReport() {
	printf("generating crash report...\n");
	Transistor::KDebug debug = ResultCode::AssertOk(
		Transistor::SVC::DebugActiveProcess(
			ResultCode::AssertOk(
				Transistor::SVC::GetProcessId(proc->handle))));
	printf("  opened debug: 0x%x\n", debug.handle);
	while(1) {
		auto r = Transistor::SVC::GetDebugEvent(debug);
		if(!r) {
			if(r.code == 0x8c01) {
				break;
			} else {
				ResultCode::AssertOk(r);
			}
		}
		switch(r->event_type) {
		case DEBUG_EVENT_ATTACH_PROCESS:
			printf("  AttachProcess\n");
			break;
		case DEBUG_EVENT_ATTACH_THREAD:
			printf("  AttachThread\n");
			break;
		case DEBUG_EVENT_UNKNOWN:
			printf("  Unknown\n");
			break;
		case DEBUG_EVENT_EXIT:
			printf("  Exit\n");
			break;
		case DEBUG_EVENT_EXCEPTION:
			printf("  Exception\n");
			printf("    Type: 0x%lx\n", r->exception.exception_type);
			break;
		}
	}
}

MonitoredProcess::~MonitoredProcess() {
}

}
