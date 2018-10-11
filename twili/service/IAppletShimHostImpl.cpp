#include "IAppletShim.hpp"

#include "IHBABIShim.hpp"

namespace twili {
namespace service {

IAppletShim::HostImpl::HostImpl(trn::ipc::server::IPCServer *server, std::shared_ptr<process::AppletProcess> process) : IAppletShim(server), process(process) {
	process->ChangeState(process::MonitoredProcess::State::Running);
}

trn::ResultCode IAppletShim::HostImpl::GetMode(trn::ipc::OutRaw<applet_shim::Mode> mode) {
	mode = applet_shim::Mode::Host;
	return RESULT_OK;
}

trn::ResultCode IAppletShim::HostImpl::SetupTarget() {
	process->SetupTarget();
	return RESULT_OK;
}

trn::ResultCode IAppletShim::HostImpl::OpenHBABIShim(trn::ipc::OutObject<IHBABIShim> &hbabishim) {
	auto r = server->CreateObject<IHBABIShim>(this, process);
	if(r) {
		hbabishim.value = r.value();
		return RESULT_OK;
	} else {
		return r.error().code;
	}
}

} // namespace service
} // namespace twili
