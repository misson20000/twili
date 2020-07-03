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

#include "TrackedProcess.hpp"
#include "AppletProcess.hpp"
#include "ProcessMonitor.hpp"

namespace twili {

class Twili;

namespace process {

/*
 * AppletTracker is responsible for managing AppletProcess
 * lifecycles and making sure they're sane and safe with regards
 * to launching (ECS) and attaching.
 */
class AppletTracker : public Tracker<AppletProcess> {
 public:
	AppletTracker(Twili &twili);

	// Tracker contract
	virtual std::shared_ptr<AppletProcess> CreateProcess() override;
	virtual void QueueLaunch(std::shared_ptr<AppletProcess> process) override;
	
	// for control process
	bool HasControlProcess();
	void AttachControlProcess();
	void ReleaseControlProcess();
	const trn::KEvent &GetProcessQueuedEvent();
	bool ReadyToLaunch();
	trn::ResultCode PopQueuedProcess(std::shared_ptr<AppletProcess> *out);
	
	// for host process
	std::shared_ptr<AppletProcess> AttachHostProcess(trn::KProcess &&process);

	void HBLLoad(std::string path, std::string argv);

	void PrintDebugInfo();

	Twili &twili;
 private:
	// an applet process has several states:
	//  - queued
	//  - created
	//  - attached

	std::deque<std::shared_ptr<AppletProcess>> queued;
	std::deque<std::shared_ptr<AppletProcess>> created;

	bool has_control_process = false;
	trn::KEvent process_queued_event;
	trn::KWEvent process_queued_wevent;

	std::shared_ptr<AppletProcess> CreateHbmenu();
	std::shared_ptr<fs::ProcessFile> hbmenu_nro;
	std::shared_ptr<AppletProcess> hbmenu;

	class Monitor : public ProcessMonitor {
	 public:
		Monitor(AppletTracker &tracker);
		virtual void StateChanged(MonitoredProcess::State new_state);
	 private:
		AppletTracker &tracker;
	} monitor;
};

} // namespace process
} // namespace twili
