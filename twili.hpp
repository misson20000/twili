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
	Transistor::Result<std::nullopt_t> Reboot(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer);
	Transistor::Result<std::nullopt_t> Run(std::vector<uint8_t> nro, usb::USBBridge::USBResponseWriter &writer);
	Transistor::Result<std::nullopt_t> CoreDump(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer);
	
	bool destroy_flag = false;
	Transistor::Waiter event_waiter;
	Transistor::IPCServer::IPCServer server;
	twili::usb::USBBridge usb_bridge;
	std::list<twili::MonitoredProcess> monitored_processes;
};

}
