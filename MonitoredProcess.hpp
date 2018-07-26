#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/waiter.hpp>

#include<memory>

#include "Process.hpp"
#include "USBBridge.hpp"
#include "ELFCrashReport.hpp"

namespace twili {

class Twili;

class MonitoredProcess : public Process {
 public:
	MonitoredProcess(Twili *twili, std::shared_ptr<trn::KProcess> proc, uint64_t target_entry);
	
	void Launch();
	void GenerateCrashReport(ELFCrashReport &report, usb::USBBridge::ResponseOpener r);
	void Terminate();
   
	std::shared_ptr<trn::KProcess> proc;
	const uint64_t target_entry;
	bool destroy_flag = false;
	bool crashed = false;
	
	~MonitoredProcess();
 private:
	Twili *twili;
	std::shared_ptr<trn::WaitHandle> wait;
};

}
