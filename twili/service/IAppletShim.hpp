#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>
#include<libtransistor/loader_config.h>

#include "applet_shim.hpp"

#include "../AppletTracker.hpp"

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
	ControlImpl(trn::ipc::server::IPCServer *server, AppletTracker &tracker);
	virtual ~ControlImpl();

	virtual trn::ResultCode GetMode(trn::ipc::OutRaw<applet_shim::Mode> mode);
	
	virtual trn::ResultCode GetEvent(trn::ipc::OutHandle<handle_t, trn::ipc::copy> event);
	virtual trn::ResultCode GetCommand(trn::ipc::OutRaw<uint32_t> cmd);
	virtual trn::ResultCode PopApplet(trn::ipc::OutObject<IAppletController> &controller);
 private:
	AppletTracker &tracker;
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
