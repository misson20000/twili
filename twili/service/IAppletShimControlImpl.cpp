#include "IAppletShim.hpp"

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
	if(tracker.HasQueuedProcess()) {
		cmd = 0;
		return RESULT_OK;
	} else {
		return TWILI_ERR_APPLET_SHIM_NO_COMMANDS_LEFT;
	}
}

trn::ResultCode IAppletShim::ControlImpl::PopApplet(trn::ipc::OutRaw<uint64_t> process_image_size) {
	process_image_size = tracker.PopQueuedProcess()->GetTargetSize();
	return RESULT_OK;
}

} // namespace service
} // namespace twili
