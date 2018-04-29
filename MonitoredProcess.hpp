#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/waiter.hpp>

#include<memory>

#include "USBBridge.hpp"
#include "ELFCrashReport.hpp"

namespace twili {

class Twili;

class MonitoredProcess {
 public:
	MonitoredProcess(Twili *twili, std::shared_ptr<Transistor::KProcess> proc);
	
	void Launch();
	Transistor::Result<std::nullopt_t> CoreDump(usb::USBBridge::USBResponseWriter &r);

	bool destroy_flag = false;
	bool crashed = false;
	
	~MonitoredProcess();
 private:
	Twili *twili;
	std::shared_ptr<Transistor::KProcess> proc;
	std::shared_ptr<Transistor::WaitHandle> wait;
};

}
