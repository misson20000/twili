#include "IAppletShim.hpp"

#include "IHBABIShim.hpp"

namespace twili {
namespace service {

IAppletShim::HostImpl::HostImpl(trn::ipc::server::IPCServer *server, std::shared_ptr<process::AppletProcess> process) : IAppletShim(server), process(process) {
}

trn::ResultCode IAppletShim::HostImpl::GetTargetSize(trn::ipc::OutRaw<size_t> size) {
	size = process->GetTargetSize();
	return RESULT_OK;
}

trn::ResultCode IAppletShim::HostImpl::SetupTarget(trn::ipc::InRaw<uint64_t> buffer_address, trn::ipc::InRaw<uint64_t> map_address) {
	process->SetupTarget(buffer_address.value, map_address.value);
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

trn::ResultCode IAppletShim::HostImpl::FinalizeTarget() {
	process->FinalizeTarget();
	return RESULT_OK;
}

} // namespace service
} // namespace twili
