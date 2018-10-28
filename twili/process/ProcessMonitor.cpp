#include "ProcessMonitor.hpp"

namespace twili {
namespace process {

ProcessMonitor::ProcessMonitor(std::shared_ptr<process::MonitoredProcess> process) : process(process) {
	process->AddMonitor(*this);
}

ProcessMonitor::~ProcessMonitor() {
	process->RemoveMonitor(*this);
}

void ProcessMonitor::StateChanged(MonitoredProcess::State new_state) {
}

} // namespace process
} // namespace twili
