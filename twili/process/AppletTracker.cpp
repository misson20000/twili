//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2019 misson20000 <xenotoad@xenotoad.net>
//
// This file is part of Twili.
//
// Twili is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Twili is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Twili.  If not, see <http://www.gnu.org/licenses/>.
//

#include "AppletTracker.hpp"

#include "../twili.hpp"
#include "AppletProcess.hpp"
#include "fs/ActualFile.hpp"

#include "err.hpp"
#include "applet_shim.hpp"

namespace twili {
namespace process {

AppletTracker::AppletTracker(Twili &twili) :
	twili(twili),
	process_queued_wevent(process_queued_event),
	monitor(*this) {
	std::shared_ptr<fs::ActualFile> af;
	twili::Assert(fs::ActualFile::Open("/sd/hbmenu.nro", &af));
	this->hbmenu_nro = std::move(af);
}

bool AppletTracker::HasControlProcess() {
	return has_control_process;
}

void AppletTracker::AttachControlProcess() {
	if(has_control_process) {
		twili::Abort(TWILI_ERR_APPLET_TRACKER_INVALID_STATE);
	}
	process_queued_wevent.Signal(); // for hbmenu launch
	has_control_process = true;
}

void AppletTracker::ReleaseControlProcess() {
	if(!has_control_process) {
		twili::Abort(TWILI_ERR_APPLET_TRACKER_INVALID_STATE);
	}
	has_control_process = false;

	printf("lost control applet. invalidating created processes...\n");
	for(std::shared_ptr<process::AppletProcess> process : created) {
		process->ChangeState(process::MonitoredProcess::State::Exited);
	}
	if(hbmenu) {
		std::shared_ptr<process::AppletProcess> hbmenu_copy = hbmenu;
		hbmenu_copy->Terminate();
		hbmenu_copy->ChangeState(process::MonitoredProcess::State::Exited);
	}
	created.clear();
}

const trn::KEvent &AppletTracker::GetProcessQueuedEvent() {
	return process_queued_event;
}

bool AppletTracker::ReadyToLaunch() {
	return
		created.size() == 0 && // there are no processes created but not yet attached
		(!monitor.process || // either there is no applet currently running
		 monitor.process->GetState() == process::MonitoredProcess::State::Exited); // or it has exited
}

trn::ResultCode AppletTracker::PopQueuedProcess(std::shared_ptr<process::AppletProcess> *out) {
	while(queued.size() > 0) {
		std::shared_ptr<process::AppletProcess> proc = queued.front();
		queued.pop_front();

		trn::ResultCode r = proc->PrepareForLaunch();
		if(r == TWILI_ERR_NO_LONGER_REQUESTED_TO_LAUNCH) { continue; }
		TWILI_CHECK(r);
		
		created.push_back(proc);
		*out = std::move(proc);
		return RESULT_OK;
	}
	
	// launch hbmenu if there's nothing else to launch
	printf("launching hbmenu\n");
	hbmenu = CreateHbmenu();

	// failure to launch hbmenu is critical
	twili::Assert(hbmenu->PrepareForLaunch());

	created.push_back(hbmenu);
	*out = std::move(hbmenu);
	return RESULT_OK;
}

std::shared_ptr<process::AppletProcess> AppletTracker::AttachHostProcess(trn::KProcess &&process) {
	printf("attaching new host process\n");

	// if we did not request creation of a host process, something has gone
	// seriously wrong and applet tracker state is inconsistent. no point in
	// recovering.
	twili::Assert<TWILI_ERR_APPLET_TRACKER_NO_PROCESS>(created.size() == 0);

	std::shared_ptr<process::AppletProcess> proc = created.front();
	proc->Attach(std::make_shared<trn::KProcess>(std::move(process)));
	printf("  attached\n");
	created.pop_front();
	monitor.Reattach(proc);
	if(ReadyToLaunch()) {
		process_queued_wevent.Signal();
	}
	return proc;
}

std::shared_ptr<AppletProcess> AppletTracker::CreateProcess() {
	return std::make_shared<AppletProcess>(*this);
}

void AppletTracker::QueueLaunch(std::shared_ptr<process::AppletProcess> process) {
	if(hbmenu) {
		// exit out of hbmenu, if it's running, before launching
		hbmenu->Kill();
	}
	queued.push_back(process);
	if(ReadyToLaunch()) {
		process_queued_wevent.Signal();
	}
}

std::shared_ptr<process::AppletProcess> AppletTracker::CreateHbmenu() {
	std::shared_ptr<process::AppletProcess> proc = CreateProcess();
	// note: we skip over the Started state through this non-standard launch procedure
	proc->AppendCode(hbmenu_nro);
	proc->argv = "sdmc:/hbmenu.nro";
	return proc;
}

void AppletTracker::HBLLoad(std::string path, std::string argv) {
	// transmute sdmc:/switch/application.nro path to /sd/switch/application.nro
	const std::string *prefix = nullptr;
	const std::string sdmc_prefix("sdmc:/");
	if(!path.compare(0, sdmc_prefix.size(), sdmc_prefix)) {
		prefix = &sdmc_prefix;
	}
	char transmute_path[0x301];
	snprintf(transmute_path, sizeof(transmute_path), "/sd/%s", path.c_str() + (prefix == nullptr ? 0 : prefix->size()));

	FILE *file = fopen(transmute_path, "rb");
	if(!file) {
		printf("  failed to open\n");
		return;
	}
	
	std::shared_ptr<process::AppletProcess> next_proc = CreateProcess();
	next_proc->AppendCode(std::make_shared<process::fs::ActualFile>(file));
	next_proc->argv = argv;
	
	printf("prepared hbl next load process. queueing...\n");
	QueueLaunch(next_proc);
}

void AppletTracker::PrintDebugInfo() {
	printf("AppletTracker debug:\n");
	printf("  has_control_process: %d\n", has_control_process);
	printf("  hbmenu: %p\n", hbmenu.get());
	if(hbmenu) {
		printf("    pid: 0x%lx\n", hbmenu->GetPid());
	}
	printf("  monitor.process: %p\n", monitor.process.get());
	if(monitor.process) {
		printf("    pid: 0x%lx\n", monitor.process->GetPid());
	}

	printf("  queued:\n");
	for(auto proc : queued) {
		auto ptr = proc.get();
		printf("    - %p\n", ptr);
		printf("      type: %s\n", typeid(*ptr).name());
		printf("      pid: 0x%lx\n", proc->GetPid());
		printf("      state: %d\n", proc->GetState());
		printf("      result: 0x%x\n", proc->GetResult().code);
		printf("      target entry: 0x%lx\n", proc->GetTargetEntry());
	}
	printf("  created:\n");
	for(auto proc : created) {
		auto ptr = proc.get();
		printf("    - %p\n", ptr);
		printf("      type: %s\n", typeid(*ptr).name());
		printf("      pid: 0x%lx\n", proc->GetPid());
		printf("      state: %d\n", proc->GetState());
		printf("      result: 0x%x\n", proc->GetResult().code);
		printf("      target entry: 0x%lx\n", proc->GetTargetEntry());
	}

}


AppletTracker::Monitor::Monitor(AppletTracker &tracker) : process::ProcessMonitor(std::shared_ptr<process::MonitoredProcess>()), tracker(tracker) {
}

void AppletTracker::Monitor::StateChanged(process::MonitoredProcess::State new_state) {
	if(tracker.ReadyToLaunch()) {
		tracker.process_queued_wevent.Signal();
	}
	if(new_state == process::MonitoredProcess::State::Exited) {
		if(process == tracker.hbmenu) {
			printf("AppletTracker: hbmenu exited\n");
			tracker.hbmenu.reset();
		}
		if(!process->next_load_path.empty()) {
			printf("AppletTracker: application requested next load: %s[%s]\n", process->next_load_path.c_str(), process->next_load_argv.c_str());
			tracker.HBLLoad(process->next_load_path, process->next_load_argv);
		}
		// we are no longer interested in monitoring the applet after it exits,
		// and we need to clear this reference pretty fast anyway since libnx
		// applets like to hog memory.
		Reattach(std::shared_ptr<process::MonitoredProcess>());
	}
}

} // namespace process
} // namespace twili
