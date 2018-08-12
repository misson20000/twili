#pragma once

#include<libtransistor/cpp/waiter.hpp>
#include<libtransistor/cpp/ipcserver.hpp>
#include "USBBridge.hpp"
#include "MonitoredProcess.hpp"

#include<list>

namespace twili {

class Twili {
 public:
	Twili();

	bool destroy_flag = false;
	trn::Waiter event_waiter;
	trn::ipc::server::IPCServer server;
	twili::usb::USBBridge usb_bridge;
	
	std::list<twili::MonitoredProcess> monitored_processes;
	std::optional<twili::MonitoredProcess*> FindMonitoredProcess(uint64_t pid);

	std::map<std::string, std::shared_ptr<TwibPipe>> named_pipes;
	
	std::vector<uint8_t> hbabi_shim_nro;
};

}
