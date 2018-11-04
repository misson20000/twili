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
	process_queued_wevent(process_queued_event) {
	printf("building AppletTracker\n");
	control_exevfs.SetMain(std::make_shared<process::fs::ActualFile>(fopen("/squash/applet_control.nso", "rb")));
	control_exevfs.SetNpdm(std::make_shared<process::fs::ActualFile>(fopen("/squash/applet_control.npdm", "rb")));
	printf("prepared control_exevfs\n");
	PrepareForControlAppletLaunch();
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
	PrepareForControlAppletLaunch();
}

const trn::KEvent &AppletTracker::GetProcessQueuedEvent() {
	return process_queued_event;
}

bool AppletTracker::ReadyToLaunch() {
	return queued.size() > 0 && created.size() == 0;
}

std::shared_ptr<process::AppletProcess> AppletTracker::PopQueuedProcess() {
	std::shared_ptr<process::AppletProcess> proc;
	while(queued.size() > 0 && !(proc = queued.front().lock())) {
		// skip dead weak pointers
		queued.pop_front();
	}
	if(queued.size() > 0) {
		created.push_back(proc);
		queued.pop_front();

		proc->PrepareForLaunch();
		return proc;
	} else {
		throw trn::ResultError(TWILI_ERR_APPLET_TRACKER_NO_PROCESS);
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
	if(ReadyToLaunch()) {
		process_queued_wevent.Signal();
	}
	return proc;
}

void AppletTracker::QueueLaunch(std::shared_ptr<process::AppletProcess> process) {
	queued.push_back(process);
	if(ReadyToLaunch()) {
		process_queued_wevent.Signal();
	}
}

void AppletTracker::PrepareForControlAppletLaunch() {
	printf("installing ExternalContentSource for control applet\n");
	trn::KObject session;
	trn::ResultCode::AssertOk(
		twili.services.ldr_shel.SendSyncRequest<65000>( // SetExternalContentSource
			trn::ipc::InRaw<uint64_t>(applet_shim::TitleId),
			trn::ipc::OutHandle<trn::KObject, trn::ipc::move>(session)));
	printf("installed ExternalContentSource for control applet\n");
	printf("  VFS server session: 0x%x\n", session.handle);
	trn::ResultCode::AssertOk(
		twili.server.AttachSession<process::fs::ProcessFileSystem::IFileSystem>(std::move(session), control_exevfs));
	printf("  session mutated: 0x%x\n", session.handle);
	printf("attached VFS server\n");
}

} // namespace twili
