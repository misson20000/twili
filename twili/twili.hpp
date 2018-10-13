#pragma once

#include<libtransistor/cpp/waiter.hpp>
#include<libtransistor/cpp/ipcserver.hpp>

#include<list>

#include "AppletTracker.hpp"
#include "process/MonitoredProcess.hpp"

#include "bridge/usb/USBBridge.hpp"
#include "bridge/tcp/TCPBridge.hpp"

#include "ipcbind/pm/IShellService.hpp"
#include "ipcbind/ldr/IDebugMonitorInterface.hpp"
#include "ipcbind/nifm/IGeneralService.hpp"

namespace twili {

class Twili {
 public:
	Twili();

	bool destroy_flag = false;
	trn::Waiter event_waiter;
	trn::ipc::server::IPCServer server;

	struct ServiceRegistration {
	 public:
		ServiceRegistration(trn::ipc::server::IPCServer &server, std::string name, std::function<trn::Result<trn::ipc::server::Object*>(trn::ipc::server::IPCServer *server)> factory);
	} twili_registration;
	
	struct Services {
	 public:
		Services();

		trn::ipc::client::Object pm_dmnt;
		service::pm::IShellService pm_shell;
		service::ldr::IDebugMonitorInterface ldr_dmnt;
		trn::ipc::client::Object ldr_shel;
		service::nifm::IGeneralService nifm;
	} services;

	struct Resources {
	 public:
		Resources();
		
		std::vector<uint8_t> hbabi_shim_nro;
	} resources;

	AppletTracker applet_tracker;
	
	bridge::usb::USBBridge usb_bridge;
	bridge::tcp::TCPBridge tcp_bridge;
	
	std::list<std::shared_ptr<process::MonitoredProcess>> monitored_processes;
	std::shared_ptr<process::MonitoredProcess> FindMonitoredProcess(uint64_t pid);
	std::shared_ptr<process::Process> FindProcess(uint64_t pid);
	
	std::map<std::string, std::shared_ptr<TwibPipe>> named_pipes;
};

} // namespace twili
