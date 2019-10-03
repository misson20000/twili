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

#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>
#include<libtransistor/loader_config.h>

#include "applet_shim.hpp"

#include "../process/AppletTracker.hpp"

namespace twili {
namespace service {

class IHBABIShim;
class IAppletController;

class IAppletShim : public trn::ipc::server::Object {
 public:
	IAppletShim(trn::ipc::server::IPCServer *server);
	virtual ~IAppletShim() = default;
	
	trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id);

	virtual trn::ResultCode GetMode(trn::ipc::OutRaw<applet_shim::Mode> mode);
	
	virtual trn::ResultCode GetEvent(trn::ipc::OutHandle<handle_t, trn::ipc::copy> event);
	virtual trn::ResultCode GetCommand(trn::ipc::OutRaw<uint32_t> cmd);
	virtual trn::ResultCode PopApplet(trn::ipc::OutObject<IAppletController> &controller);

	virtual trn::ResultCode OpenHBABIShim(trn::ipc::OutObject<IHBABIShim> &hbabishim);

	class ControlImpl;
	class HostImpl;
};

class IAppletShim::ControlImpl : public IAppletShim {
 public:
	ControlImpl(trn::ipc::server::IPCServer *server, process::AppletTracker &tracker);
	virtual ~ControlImpl();

	virtual trn::ResultCode GetMode(trn::ipc::OutRaw<applet_shim::Mode> mode);
	
	virtual trn::ResultCode GetEvent(trn::ipc::OutHandle<handle_t, trn::ipc::copy> event);
	virtual trn::ResultCode GetCommand(trn::ipc::OutRaw<uint32_t> cmd);
	virtual trn::ResultCode PopApplet(trn::ipc::OutObject<IAppletController> &controller);
 private:
	process::AppletTracker &tracker;
};

class IAppletShim::HostImpl : public IAppletShim {
 public:
	HostImpl(trn::ipc::server::IPCServer *server, std::shared_ptr<process::AppletProcess> process);

	virtual trn::ResultCode GetMode(trn::ipc::OutRaw<applet_shim::Mode> mode);
	
	virtual trn::ResultCode OpenHBABIShim(trn::ipc::OutObject<IHBABIShim> &hbabishim);
 private:
	std::shared_ptr<process::AppletProcess> process;
};

} // namespace service
} // namespace twili
