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

	// bridge commands
	bool Reboot();
	bool Run(std::vector<uint8_t> nro);
	
	bool destroy_flag = false;
	Transistor::Waiter event_waiter;
	Transistor::IPCServer::IPCServer server;
	twili::usb::USBBridge usb_bridge;
	std::list<twili::MonitoredProcess> monitored_processes;
};

}
