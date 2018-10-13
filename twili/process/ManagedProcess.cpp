#include "ManagedProcess.hpp"

#include "../twili.hpp"

#include "../process_creation.hpp"

using namespace trn;

namespace twili {
namespace process {

ManagedProcess::ManagedProcess(Twili &twili, bridge::ResponseOpener attachment_opener, std::vector<uint8_t> nro) : MonitoredProcess(twili, attachment_opener) {
	std::vector<uint32_t> caps = {
		0b00011111111111111111111111101111, // SVC grants
		0b00111111111111111111111111101111,
		0b01011111111111111111111111101111,
		0b01100000000000001111111111101111,
		0b10011111111111111111111111101111,
		0b10100000000000000000111111101111,
		0b00000010000000000111001110110111, // KernelFlags
		0b00000000000000000101111111111111, // ApplicationType
		0b00000000000110000011111111111111, // KernelReleaseVersion
		0b00000010000000000111111111111111, // HandleTableSize
		0b00000000000001101111111111111111, // DebugFlags (can be debugged)
	};

	twili::process_creation::ProcessBuilder builder;
	process_creation::ProcessBuilder::VectorDataReader hbabi_shim_reader(twili.resources.hbabi_shim_nro);
	process_creation::ProcessBuilder::VectorDataReader nro_reader(nro);
	uint64_t   shim_addr = ResultCode::AssertOk(builder.AppendNRO(hbabi_shim_reader));
	
	target_entry = ResultCode::AssertOk(builder.AppendNRO(nro_reader));
	std::shared_ptr<trn::KProcess> process = ResultCode::AssertOk(builder.Build("twili_child", caps));

	Attach(process);
	
	printf("created managed process: 0x%x, pid 0x%x\n", proc->handle, GetPid());
	wait = twili.event_waiter.Add(*proc, [this]() {
			printf("managed process (0x%x) signalled\n", this->proc->handle);
			this->proc->ResetSignal();
			auto state = (trn::svc::ProcessState) ResultCode::AssertOk(trn::svc::GetProcessInfo(*this->proc, 0));
			printf("  state: 0x%lx\n", state);
			if(state == trn::svc::ProcessState::CRASHED) {
				printf("managed process (0x%x) crashed\n", this->proc->handle);
				printf("ready to generate crash report\n");
				ChangeState(State::Crashed);
			}
			if(state == trn::svc::ProcessState::EXITED) {
				printf("managed process (0x%x) exited\n", this->proc->handle);
				ChangeState(State::Exited);
			}
			return true;
		});
}

void ManagedProcess::Launch() {
	printf("launching managed process: 0x%x\n", proc->handle);
	auto r = trn::svc::StartProcess(*proc, 58, 0, 0x100000);
	if(!r) {
		SetResult(r.error());
		ChangeState(State::Exited);
	} else {
		ChangeState(State::Running);
	}
}

} // namespace process
} // namespace twili
