#include "AppletTracker.hpp"

#include "twili.hpp"
#include "process/AppletProcess.hpp"

#include "err.hpp"

namespace twili {

AppletTracker::AppletTracker(Twili &twili) :
	twili(twili),
	process_queued_wevent(process_queued_event) {
}

bool AppletTracker::HasControlProcess() {
	return has_control_process;
}

void AppletTracker::AttachControlProcess() {
	if(has_control_process) {
		throw trn::ResultError(TWILI_ERR_APPLET_TRACKER_INVALID_STATE);
	}
	has_control_process = true;
}

void AppletTracker::ReleaseControlProcess() {
	if(!has_control_process) {
		throw trn::ResultError(TWILI_ERR_APPLET_TRACKER_INVALID_STATE);
	}
	has_control_process = false;
}

const trn::KEvent &AppletTracker::GetProcessQueuedEvent() {
	return process_queued_event;
}

bool AppletTracker::PopQueuedProcess() {
	if(queued.size() > 0) {
		created.push_back(queued.front());
		queued.pop_front();
		return true;
	} else {
		return false;
	}
}

std::shared_ptr<process::AppletProcess> AppletTracker::AttachHostProcess(trn::KProcess &&process) {
	printf("attaching new host process\n");
	if(created.size() == 0) {
		printf("  no processes created\n");
		throw trn::ResultError(TWILI_ERR_APPLET_TRACKER_NO_PROCESS);
	}
	std::shared_ptr<process::AppletProcess> proc = created.front();
	proc->Attach(std::move(process));
	printf("  attached\n");
	twili.monitored_processes.push_back(proc);
	printf("  began monitoring 0x%x\n", proc->GetPid());
	created.pop_front();
	return proc;
}

void AppletTracker::QueueLaunch(std::shared_ptr<process::AppletProcess> process) {
	queued.push_back(process);
	process_queued_wevent.Signal();
}

} // namespace twili
