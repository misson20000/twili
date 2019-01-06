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

#include<memory>
#include<optional>
#include<deque>

#include "MonitoredProcess.hpp"
#include "fs/ProcessFile.hpp"
#include "fs/ProcessFileSystem.hpp"

namespace twili {

class AppletTracker;

namespace process {

class AppletProcess : public MonitoredProcess {
 public:
	AppletProcess(Twili &twili);
	
	virtual void Launch(bridge::ResponseOpener) override;
	virtual void AddHBABIEntries(std::vector<loader_config_entry_t> &entries) override;
	virtual void ChangeState(State state) override;
	virtual void Kill() override;
	
	// sets up ExternalContentSource for loader
	void PrepareForLaunch();

	// used to communicate with host shim
	trn::KEvent &GetCommandEvent();
	uint32_t PopCommand();

	fs::ProcessFileSystem virtual_exefs;
 private:
	std::optional<bridge::ResponseOpener> run_opener;

	std::shared_ptr<trn::WaitHandle> kill_timeout;

	void PushCommand(uint32_t command);
	trn::KEvent command_event;
	trn::KWEvent command_wevent;
	std::deque<uint32_t> commands;
};

} // namespace process
} // namespace twili
