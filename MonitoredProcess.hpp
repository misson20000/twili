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
	MonitoredProcess(Twili *twili, std::shared_ptr<trn::KProcess> proc, uint64_t target_entry);
	
	void Launch();
	trn::Result<std::nullopt_t> CoreDump(usb::USBBridge::USBResponseWriter &r);
   trn::Result<std::nullopt_t> Terminate();
   
	std::shared_ptr<trn::KProcess> proc;
	const uint64_t target_entry;
	const uint64_t pid;
	bool destroy_flag = false;
	bool crashed = false;
	
	~MonitoredProcess();
 private:
	Twili *twili;
	std::shared_ptr<trn::WaitHandle> wait;
};

}
