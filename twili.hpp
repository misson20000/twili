#pragma once

#include<libtransistor/cpp/waiter.hpp>
#include<libtransistor/cpp/ipcserver.hpp>

#include<list>

#include "MonitoredProcess.hpp"

#include "bridge/usb/USBBridge.hpp"
#include "bridge/tcp/TCPBridge.hpp"

#include "service/pm/IShellService.hpp"
#include "service/ldr/IDebugMonitorInterface.hpp"
#include "service/nifm/IGeneralService.hpp"

namespace twili {

class Twili {
 public:
	Twili();

	struct Services {
	 public:
		Services();
		
		service::pm::IShellService pm_shell;
		service::ldr::IDebugMonitorInterface ldr_dmnt;
		service::nifm::IGeneralService nifm;
	} services;
	
	bool destroy_flag = false;
	trn::Waiter event_waiter;
	trn::ipc::server::IPCServer server;
	bridge::usb::USBBridge usb_bridge;
	bridge::tcp::TCPBridge tcp_bridge;
	
	std::list<twili::MonitoredProcess> monitored_processes;
	std::optional<twili::MonitoredProcess*> FindMonitoredProcess(uint64_t pid);

	std::map<std::string, std::shared_ptr<TwibPipe>> named_pipes;
	
	std::vector<uint8_t> hbabi_shim_nro;
};

}
