#pragma once

#include "MonitoredProcess.hpp"

namespace twili {
namespace process {

class ProcessMonitor {
 public:
	ProcessMonitor(std::shared_ptr<process::MonitoredProcess> process);
	~ProcessMonitor();

	// don't copy or move us please, since process keeps track of us via pointer
	ProcessMonitor(const ProcessMonitor &other) = delete;
	ProcessMonitor(ProcessMonitor &&other) = delete;
	
	virtual void StateChanged(MonitoredProcess::State new_state);
	
 protected:
	std::shared_ptr<process::MonitoredProcess> process;
};

} // namespace process
} // namespace twili
