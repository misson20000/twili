#include "IAppletShim.hpp"

#include "../twili.hpp"
#include "../process/MonitoredProcess.hpp"

#include "IHBABIShim.hpp"
#include "IAppletController.hpp"

#include "err.hpp"

namespace twili {
namespace service {

IAppletShim::IAppletShim(trn::ipc::server::IPCServer *server) : trn::ipc::server::Object(server) {
	printf("opening applet shim\n");
}

trn::ResultCode IAppletShim::Dispatch(trn::ipc::Message msg, uint32_t request_id) {
	switch(request_id) {
	case 0:
		return trn::ipc::server::RequestHandler<&IAppletShim::GetMode>::Handle(this, msg);
	case 100:
		return trn::ipc::server::RequestHandler<&IAppletShim::GetEvent>::Handle(this, msg);
	case 101:
		return trn::ipc::server::RequestHandler<&IAppletShim::GetCommand>::Handle(this, msg);
	case 102:
		return trn::ipc::server::RequestHandler<&IAppletShim::PopApplet>::Handle(this, msg);
	case 201:
		return trn::ipc::server::RequestHandler<&IAppletShim::SetupTarget>::Handle(this, msg);
	case 202:
		return trn::ipc::server::RequestHandler<&IAppletShim::OpenHBABIShim>::Handle(this, msg);
	}
	return 1;
}

trn::ResultCode IAppletShim::GetMode(trn::ipc::OutRaw<applet_shim::Mode> mode) {
	return TWILI_ERR_APPLET_SHIM_UNKNOWN_MODE;
}

trn::ResultCode IAppletShim::GetEvent(trn::ipc::OutHandle<handle_t, trn::ipc::copy> event) {
	return TWILI_ERR_APPLET_SHIM_WRONG_MODE;
}

trn::ResultCode IAppletShim::GetCommand(trn::ipc::OutRaw<uint32_t> cmd) {
	return TWILI_ERR_APPLET_SHIM_WRONG_MODE;
}

trn::ResultCode IAppletShim::PopApplet(trn::ipc::OutObject<IAppletController> &controller) {
	return TWILI_ERR_APPLET_SHIM_WRONG_MODE;
}

trn::ResultCode IAppletShim::SetupTarget() {
	return TWILI_ERR_APPLET_SHIM_WRONG_MODE;
}

trn::ResultCode IAppletShim::OpenHBABIShim(trn::ipc::OutObject<IHBABIShim> &hbabishim) {
	return TWILI_ERR_APPLET_SHIM_WRONG_MODE;
}

} // namespace service
} // namespace twili
