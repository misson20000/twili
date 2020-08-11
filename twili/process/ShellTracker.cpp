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
#include "../Services.hpp"
#include "ShellProcess.hpp"
#include "fs/ActualFile.hpp"

#include "err.hpp"
#include "title_id.hpp"

using namespace trn;

namespace twili {
namespace process {

ShellTracker::ShellTracker(Twili &twili) :
	twili(twili) {
	printf("in shell tracker constructor\n");
	
	twili::Assert(twili.services->GetShellEventHandle(&shell_event));

	printf("got shell event handle\n");

	/* Use main thread waiter here, since most things don't synchronize. */
	shell_wait = twili.event_waiter.Add(
		shell_event,
		[this]() {
			std::unique_lock<thread::Mutex> lock(queue_mutex);
			shell_event.ResetSignal();

			std::optional<hos_types::ShellEventInfo> r;
			// our IPC command wrapper converts appropriate "no more events" responses into empty optional
			while(twili::Assert(this->twili.services->GetShellEventInfo(&r)), r) {
				hos_types::ShellEventInfo sei = *r;
				
				auto i = tracking.find(sei.pid);
				printf("got notification %d for pid 0x%lx\n", (int) sei.event, sei.pid);

				switch(sei.event) {
				case hos_types::ShellEventType::Exit:
					printf("got exit notification for 0x%lx\n", sei.pid);
					if(i != tracking.end()) {
						printf("  marking as exited\n");
						i->second->ChangeState(MonitoredProcess::State::Exited);
						tracking.erase(i);
					}
					break;
				/* TODO
				case hos_types::ShellEventType::Crash:
					printf("got crash notification for 0x%lx\n", sei.pid);
					if(i != tracking.end()) {
						printf("  marking as crashed\n");
						i->second->ChangeState(MonitoredProcess::State::Crashed);
					}
					break; */
				case hos_types::ShellEventType::Launch:
					printf("got launch notification for 0x%lx\n", sei.pid);
					// don't care
					break;
				default:
					printf("unknown shell event for 0x%lx: %d\n", sei.pid, (int) sei.event);
				}
			}
			
			return true;
		});

	printf("added shell event to main waiter\n");

	/* Need to punt this off to another thread so we don't block IPC server. */
	tracker_signal = event_thread.event_waiter.AddSignal(
		[this]() {
			std::unique_lock<thread::Mutex> l(queue_mutex);
			tracker_signal->ResetSignal();
			TryToLaunch();
			return true;
		});

	printf("added signal to shell tracker waiter\n");

	event_thread.Start();
}

ShellTracker::~ShellTracker() {
	printf("destroying shell tracker (is stack unwinding?)\n");
	event_thread.StopSync();
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
		auto p = proc.get();
		printf("    - %p\n", p);
		printf("      type: %s\n", typeid(*p).name());
		printf("      pid: 0x%lx\n", proc->GetPid());
		printf("      state: %d\n", proc->GetState());
		printf("      result: 0x%x\n", proc->GetResult().code);
		printf("      target entry: 0x%lx\n", proc->GetTargetEntry());
	}
	printf("  created:\n");
	for(auto proc : created) {
		auto p = proc.get();
		printf("    - %p\n", p);
		printf("      type: %s\n", typeid(*p).name());
		printf("      pid: 0x%lx\n", proc->GetPid());
		printf("      state: %d\n", proc->GetState());
		printf("      result: 0x%x\n", proc->GetResult().code);
		printf("      target entry: 0x%lx\n", proc->GetTargetEntry());
	}
}

bool ShellTracker::ReadyToLaunch() {
	return created.size() == 0; // no processes waiting in limbo
}

trn::ResultCode ShellTracker::PopQueuedProcess(std::shared_ptr<ShellProcess> *out) {
	while(queued.size() > 0) {
		std::shared_ptr<ShellProcess> proc = queued.front();
		queued.pop_front();

		trn::ResultCode r = proc->PrepareForLaunch();
		if(r == TWILI_ERR_NO_LONGER_REQUESTED_TO_LAUNCH) { continue; }
		TWILI_CHECK(r);

		created.push_back(proc);
		*out = std::move(proc);
		return RESULT_OK;
	}

	return TWILI_ERR_NO_LONGER_REQUESTED_TO_LAUNCH;
}

std::shared_ptr<ShellProcess> ShellTracker::AttachHostProcess(trn::KProcess &&process) {
	std::shared_ptr<process::ShellProcess> proc;
	{
		std::unique_lock<thread::Mutex> l(queue_mutex);
	
		// if we did not request creation of a host process, something has gone
		// seriously wrong and shell tracker state is inconsistent. no point in
		// recovering.
		twili::Assert<TWILI_ERR_TRACKER_NO_PROCESSES_CREATED>(created.size() != 0);
		
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

	std::shared_ptr<ShellProcess> proc;
	trn::ResultCode r = PopQueuedProcess(&proc);
	
	if(r == TWILI_ERR_NO_LONGER_REQUESTED_TO_LAUNCH || !proc) {
		return;
	}

	uint64_t pid;
	
	if(r == RESULT_OK) {
		r = RequestLaunchECSProgram(&pid);
	}

	if(r == RESULT_OK) {
		printf("  => OK\n");
		tracking.emplace(std::make_pair(pid, proc));
	} else {
		printf("  => 0x%x\n", r.code);
		proc->SetResult(r);
		proc->ChangeState(MonitoredProcess::State::Exited);
		created.clear();
	}
}

trn::ResultCode ShellTracker::RequestLaunchECSProgram(uint64_t *pid_out) {
	const uint8_t storage_id = 0; // none

	uint32_t launch_flags;
	if(env_get_kernel_version() >= KERNEL_VERSION_500) {
		launch_flags = 0xb; // notify exit, notify debug, notify debug special
	} else {
		launch_flags = 0x31; // notify exit, notify debug, notify debug special
	}

	return twili.services->LaunchProgram(launch_flags, title_id::ShellProcessDefaultTitle, storage_id, pid_out);
}

} // namespace process
} // namespace twili
