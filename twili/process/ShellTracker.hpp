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

#pragma once

#include<libtransistor/cpp/nx.hpp>

#include<deque>

#include "../Threading.hpp"
#include "TrackedProcess.hpp"
#include "ShellProcess.hpp"

namespace twili {

class Twili;

namespace process {

/*
 * ShellTracker is responsible for managing shell process
 * lifecycles, making sure they're safe w.r.t ECS, and negotiating
 * to NS.
 */
class ShellTracker : public Tracker<ShellProcess> {
 public:
	ShellTracker(Twili &twili);

	// Tracker contract
	virtual std::shared_ptr<ShellProcess> CreateProcess() override;
	virtual void QueueLaunch(std::shared_ptr<ShellProcess> process) override;

	// service
	std::shared_ptr<ShellProcess> AttachHostProcess(trn::KProcess &&process);
	
	void PrintDebugInfo();

	Twili &twili;
 private:
	trn::KEvent shell_event;
	std::shared_ptr<trn::WaitHandle> shell_wait;
	std::shared_ptr<trn::WaitHandle> tracker_signal;
	
	// a shell process has several states:
	//  - queued
	//  - created
	//  - attached

	thread::Mutex queue_mutex;
	std::deque<std::shared_ptr<ShellProcess>> queued GUARDED_BY(queue_mutex);
	std::deque<std::shared_ptr<ShellProcess>> created GUARDED_BY(queue_mutex);
	std::map<uint64_t, std::shared_ptr<ShellProcess>> tracking GUARDED_BY(queue_mutex);
	
	bool ReadyToLaunch() REQUIRES(queue_mutex);
	std::shared_ptr<ShellProcess> PopQueuedProcess() REQUIRES(queue_mutex);
	void TryToLaunch() REQUIRES(queue_mutex);

	// ns:dev launches processes synchronously, but we need to serve ECS filesystem
	// so we can't block main IPC server thread on process launch.
	
	// make sure to put this near the end so it destructs early
	thread::EventThread event_thread;
};

} // namespace process
} // namespace twili
