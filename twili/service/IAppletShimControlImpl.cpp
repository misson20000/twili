//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2018 misson20000 <xenotoad@xenotoad.net>
//
// This file is part of Twili.
//
// Twili is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Twili is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Twili.  If not, see <http://www.gnu.org/licenses/>.
//

#include "IAppletShim.hpp"

#include "IAppletController.hpp"

#include "../twili.hpp"
#include "err.hpp"

namespace twili {
namespace service {

IAppletShim::ControlImpl::ControlImpl(trn::ipc::server::IPCServer *server, process::AppletTracker &tracker) : IAppletShim(server), tracker(tracker) {
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
	std::shared_ptr<process::AppletProcess> proc;
	TWILI_CHECK(tracker.PopQueuedProcess(&proc));
	auto r = server->CreateObject<IAppletController>(this, std::move(proc));
	if(r) {
		controller.value = r.value();
		return RESULT_OK;
	} else {
		return r.error().code;
	}
}

} // namespace service
} // namespace twili
