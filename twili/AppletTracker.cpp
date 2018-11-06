//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2018 misson20000 <xenotoad@xenotoad.net>
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

#include "twili.hpp"
#include "process/AppletProcess.hpp"
#include "process/fs/ActualFile.hpp"

#include "err.hpp"
#include "applet_shim.hpp"

namespace twili {

AppletTracker::AppletTracker(Twili &twili) :
	twili(twili),
	process_queued_wevent(process_queued_event),
	monitor(*this) {
	printf("building AppletTracker\n");
	hbmenu_transmute = process::fs::NRONSOTransmutationFile::Create(
		std::make_shared<process::fs::ActualFile>("/sd/hbmenu.nro"));
	process_queued_wevent.Signal(); // for hbmenu launch
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

bool AppletTracker::ReadyToLaunch() {
	return
		created.size() == 0 && // there are no processes created but not yet attached
		(!monitor.process || // either there is no applet currently running
		 monitor.process->GetState() == process::MonitoredProcess::State::Exited); // or it has exited
}

std::shared_ptr<process::AppletProcess> AppletTracker::PopQueuedProcess() {
	if(queued.size() > 0) {
		std::shared_ptr<process::AppletProcess> proc = queued.front();
		created.push_back(proc);
		queued.pop_front();

		proc->PrepareForLaunch();
		return proc;
	} else {
		// launch hbmenu if there's nothing else to launch
		printf("launching hbmenu\n");
		hbmenu = CreateHbmenu();
		created.push_back(hbmenu);
		hbmenu->PrepareForLaunch();
		return hbmenu;
	}
}

std::shared_ptr<process::AppletProcess> AppletTracker::AttachHostProcess(trn::KProcess &&process) {
	printf("attaching new host process\n");
	if(created.size() == 0) {
		printf("  no processes created\n");
		throw trn::ResultError(TWILI_ERR_APPLET_TRACKER_NO_PROCESS);
	}
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
	std::shared_ptr<process::AppletProcess> proc = std::make_shared<process::AppletProcess>(twili);
	// note: we skip over the Started state through this non-standard launch procedure

	proc->virtual_exefs.SetMain(hbmenu_transmute);
	return proc;
}

AppletTracker::Monitor::Monitor(AppletTracker &tracker) : process::ProcessMonitor(std::shared_ptr<process::MonitoredProcess>()), tracker(tracker) {
}

void AppletTracker::Monitor::StateChanged(process::MonitoredProcess::State new_state) {
	if(tracker.ReadyToLaunch()) {
		tracker.process_queued_wevent.Signal();
	}
	if(process == tracker.hbmenu && new_state == process::MonitoredProcess::State::Exited) {
		tracker.hbmenu.reset();
		Reattach(std::shared_ptr<process::MonitoredProcess>()); // clear this reference since hbmenu likes to hog memory
	}
}

} // namespace twili
