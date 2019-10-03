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

#include "ShellTracker.hpp"

#include<mutex>

#include "../twili.hpp"
#include "ShellProcess.hpp"
#include "fs/ActualFile.hpp"

#include "err.hpp"
#include "title_id.hpp"

using namespace trn;

namespace twili {
namespace process {

ShellTracker::ShellTracker(Twili &twili) :
	twili(twili) {

	handle_t evt_h;
	ResultCode::AssertOk(
		twili.services.ns_dev.SendSyncRequest<4>( // GetShellEventHandle
			ipc::OutHandle<handle_t, ipc::copy>(evt_h)));
	shell_event = KEvent(evt_h);
	
	shell_wait = event_thread.event_waiter.Add(
		shell_event,
		[this]() {
			std::unique_lock<thread::Mutex> lock(queue_mutex);
			shell_event.ResetSignal();
			uint64_t evt;
			uint64_t pid;
			Result<std::nullopt_t> r = std::nullopt;
			while(
				(r = this->twili.services.ns_dev.SendSyncRequest<5>( // GetShellEventInfo
					ipc::OutRaw<uint64_t>(evt),
					ipc::OutRaw<uint64_t>(pid)))) {
				auto i = tracking.find(pid);
				if(evt == 1) { // exit
					printf("got exit notification for 0x%lx\n", pid);
					if(i != tracking.end()) {
						printf("  marking as exited\n");
						i->second->ChangeState(MonitoredProcess::State::Exited);
						tracking.erase(i);
					}
				} else if(evt == 2) { // crash
					printf("got crash notification for 0x%lx\n", pid);
					if(i != tracking.end()) {
						printf("  marking as crashed\n");
						i->second->ChangeState(MonitoredProcess::State::Crashed);
					}
				} else if(evt == 4) { // launch
					printf("got launch notification for 0x%lx\n", pid);
					// don't care
				} else {
					printf("Unknown ns:dev shell event for 0x%lx: %d\n", pid, evt);
				}
			}
			if(r.error().code != 0x610) { // no events left
				ResultCode::AssertOk(std::move(r));
			}
			return true;
		});

	tracker_signal = event_thread.event_waiter.AddSignal(
		[this]() {
			std::unique_lock<thread::Mutex> l(queue_mutex);
			tracker_signal->ResetSignal();
			TryToLaunch();
			return true;
		});

	event_thread.Start();
}

std::shared_ptr<ShellProcess> ShellTracker::CreateProcess() {
	return std::make_shared<ShellProcess>(*this);
}

void ShellTracker::QueueLaunch(std::shared_ptr<process::ShellProcess> process) {
	std::unique_lock<thread::Mutex> l(queue_mutex);
	
	queued.push_back(process);
	tracker_signal->Signal(); // try to launch
}

void ShellTracker::PrintDebugInfo() {
	std::unique_lock<thread::Mutex> l(queue_mutex);
	
	printf("ShellTracker debug:\n");
	printf("  queued:\n");
	for(auto proc : queued) {
		printf("    - %p\n", proc.get());
		printf("      type: %s\n", typeid(*proc.get()).name());
		printf("      pid: 0x%lx\n", proc->GetPid());
		printf("      state: %d\n", proc->GetState());
		printf("      result: 0x%x\n", proc->GetResult().code);
		printf("      target entry: 0x%lx\n", proc->GetTargetEntry());
	}
	printf("  created:\n");
	for(auto proc : created) {
		printf("    - %p\n", proc.get());
		printf("      type: %s\n", typeid(*proc.get()).name());
		printf("      pid: 0x%lx\n", proc->GetPid());
		printf("      state: %d\n", proc->GetState());
		printf("      result: 0x%x\n", proc->GetResult().code);
		printf("      target entry: 0x%lx\n", proc->GetTargetEntry());
	}
}

bool ShellTracker::ReadyToLaunch() {
	return created.size() == 0; // no processes waiting in limbo
}

std::shared_ptr<ShellProcess> ShellTracker::PopQueuedProcess() {
	while(queued.size() > 0) {
		std::shared_ptr<ShellProcess> proc = queued.front();
		queued.pop_front();

		// returns true if process wasn't cancelled,
		// but if it returns false, attempt to dequeue
		// another process
		if(proc->PrepareForLaunch()) {
			created.push_back(proc);
			return proc;
		}
	}
	
	return std::shared_ptr<ShellProcess>();
}

std::shared_ptr<ShellProcess> ShellTracker::AttachHostProcess(trn::KProcess &&process) {
	std::shared_ptr<process::ShellProcess> proc;
	{
		std::unique_lock<thread::Mutex> l(queue_mutex);
	
		if(created.size() == 0) {
			printf("  AHP: no processes created\n");
			throw trn::ResultError(TWILI_ERR_APPLET_TRACKER_NO_PROCESS);
		}
		proc = created.front();
		proc->Attach(std::make_shared<trn::KProcess>(std::move(process)));
		created.pop_front();
		tracker_signal->Signal(); // try launch
	}
	return proc;
}

void ShellTracker::TryToLaunch() {
	if(!ReadyToLaunch()) {
		return;
	}

	std::shared_ptr<ShellProcess> proc = PopQueuedProcess();
	if(!proc) {
		return;
	}

	const uint8_t storage_id = 3; // nand system
	
	printf("redirecting path...\n");
	ipc::client::Object ilr;
	ResultCode::AssertOk(
		twili.services.lr.SendSyncRequest<0>( // OpenLocationResolver
			ipc::InRaw<uint8_t>(storage_id),
			ipc::OutObject(ilr)));
	char path[] = "@Sdcard://doesntexist.nsp";
	ResultCode::AssertOk(
		ilr.SendSyncRequest<1>( // RedirectProgramPath
			ipc::InRaw<uint64_t>(title_id::ShellProcessDefaultTitle),
			ipc::Buffer<char, 0x19>(path, sizeof(path))));
	
	printf("requesting to launch...\n");
	uint32_t launch_flags;
	if(env_get_kernel_version() >= KERNEL_VERSION_500) {
		launch_flags = 0xb; // notify exit, notify debug, notify debug special
	} else {
		launch_flags = 0x31; // notify exit, notify debug, notify debug special
	}
	uint64_t pid;
	auto r =
		twili.services.ns_dev.SendSyncRequest<0>( // LaunchProgram
			ipc::InRaw<uint32_t>(launch_flags),
			ipc::InRaw<uint64_t>(title_id::ShellProcessDefaultTitle),
			ipc::InRaw<uint32_t>(0), // padding???
			ipc::InRaw<uint32_t>(storage_id),
			ipc::OutRaw<uint64_t>(pid)
			);
	printf("requested launch\n");
	if(r) {
		printf("  => OK\n");
		tracking.emplace(std::make_pair(pid, proc));
	} else {
		printf("  => 0x%x\n", r.error().code);
		proc->SetResult(r.error());
		proc->ChangeState(MonitoredProcess::State::Exited);
		created.clear();
	}
}

} // namespace process
} // namespace twili
