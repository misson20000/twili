#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/waiter.hpp>

#include<memory>

namespace twili {

class Twili;

class MonitoredProcess {
 public:
	MonitoredProcess(Twili *twili, std::shared_ptr<Transistor::KProcess> proc);
	
	void Launch();

	bool destroy_flag = false;
	
	~MonitoredProcess();
 private:
	Twili *twili;
	std::shared_ptr<Transistor::KProcess> proc;
	std::shared_ptr<Transistor::WaitHandle> wait;
};

}
