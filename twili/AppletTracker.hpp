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

#pragma once

#include<libtransistor/cpp/nx.hpp>

#include<deque>

#include "process/AppletProcess.hpp"
#include "process/ProcessMonitor.hpp"
#include "process/fs/NRONSOTransmutationFile.hpp"

namespace twili {

class Twili;

class AppletTracker {
 public:
	AppletTracker(Twili &twili);
	
	// for control process
	bool HasControlProcess();
	void AttachControlProcess();
	void ReleaseControlProcess();
	const trn::KEvent &GetProcessQueuedEvent();
	bool ReadyToLaunch();
	std::shared_ptr<process::AppletProcess> PopQueuedProcess();
	
	// for host process
	std::shared_ptr<process::AppletProcess> AttachHostProcess(trn::KProcess &&process);

	// for twili
	void QueueLaunch(std::shared_ptr<process::AppletProcess> process);

	void HBLLoad(std::string path, std::string argv);
 private:
	Twili &twili;
	
	// an applet process has several states:
	//  - queued
	//  - created
	//  - attached

	std::deque<std::shared_ptr<process::AppletProcess>> queued;
	std::deque<std::shared_ptr<process::AppletProcess>> created;

	bool has_control_process = false;
	trn::KEvent process_queued_event;
	trn::KWEvent process_queued_wevent;

	std::shared_ptr<process::AppletProcess> CreateHbmenu();
	std::shared_ptr<process::fs::NSOTransmutationFile> hbmenu_transmute;
	std::shared_ptr<process::AppletProcess> hbmenu;

	class Monitor : public process::ProcessMonitor {
	 public:
		Monitor(AppletTracker &tracker);
		virtual void StateChanged(process::MonitoredProcess::State new_state);
	 private:
		AppletTracker &tracker;
	} monitor;
};

} // namespace twili
