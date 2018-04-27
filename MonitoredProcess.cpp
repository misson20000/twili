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
			auto state = ResultCode::AssertOk(Transistor::SVC::GetProcessInfo(*this->proc, 0));
			printf("  state: 0x%lx\n", state);
			if(state == (uint64_t) Transistor::SVC::ProcessState::EXITED) {
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

MonitoredProcess::~MonitoredProcess() {
}

}
