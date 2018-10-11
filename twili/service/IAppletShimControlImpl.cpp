#include "IAppletShim.hpp"

#include "IAppletController.hpp"

#include "err.hpp"

namespace twili {
namespace service {

IAppletShim::ControlImpl::ControlImpl(trn::ipc::server::IPCServer *server, AppletTracker &tracker) : IAppletShim(server), tracker(tracker) {
	tracker.AttachControlProcess();
}

IAppletShim::ControlImpl::~ControlImpl() {
	tracker.ReleaseControlProcess();
}

trn::ResultCode IAppletShim::ControlImpl::GetMode(trn::ipc::OutRaw<applet_shim::Mode> mode) {
	mode = applet_shim::Mode::Control;
	return RESULT_OK;
}

trn::ResultCode IAppletShim::ControlImpl::GetEvent(trn::ipc::OutHandle<handle_t, trn::ipc::copy> event) {
	event = tracker.GetProcessQueuedEvent().handle;
	return RESULT_OK;
}

trn::ResultCode IAppletShim::ControlImpl::GetCommand(trn::ipc::OutRaw<uint32_t> cmd) {
	if(tracker.ReadyToLaunch()) {
		cmd = 0;
		return RESULT_OK;
	} else {
		return TWILI_ERR_APPLET_SHIM_NO_COMMANDS_LEFT;
	}
}

trn::ResultCode IAppletShim::ControlImpl::PopApplet(trn::ipc::OutObject<IAppletController> &controller) {
	auto r = server->CreateObject<IAppletController>(this, std::move(tracker.PopQueuedProcess()));
	if(r) {
		controller.value = r.value();
		return RESULT_OK;
	} else {
		return r.error().code;
	}
	return RESULT_OK;
}

} // namespace service
} // namespace twili
