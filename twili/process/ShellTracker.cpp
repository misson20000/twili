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
	
	shell_event = ResultCode::AssertOk(twili.services->GetShellEventHandle());

	printf("got shell event handle\n");
	
	shell_wait = event_thread.event_waiter.Add(
		shell_event,
		[this]() {
			std::unique_lock<thread::Mutex> lock(queue_mutex);
			shell_event.ResetSignal();

			std::optional<hos_types::ShellEventInfo> r;
			while((r = ResultCode::AssertOk(this->twili.services->GetShellEventInfo()))) {
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

	printf("added shell event to shell tracker waiter\n");

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
			twili::Abort(TWILI_ERR_APPLET_TRACKER_NO_PROCESS);
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
	ipc::client::Object ilr = ResultCode::AssertOk(twili.services->OpenLocationResolver(storage_id));

	// This path points to an empty nsp. It just needs to exist to make
	// loader happy...
	char path[] = "@Sdcard://atmosphere/contents/0100000000006482/exefs.nsp";
	
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

	printf("requested launch\n");
	auto r = twili.services->LaunchProgram(launch_flags, title_id::ShellProcessDefaultTitle, storage_id);
	if(r) {
		printf("  => OK\n");
		tracking.emplace(std::make_pair(*r, proc));
	} else {
		printf("  => 0x%x\n", r.error().code);
		proc->SetResult(r.error());
		proc->ChangeState(MonitoredProcess::State::Exited);
		created.clear();
	}
}

} // namespace process
} // namespace twili
