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

#include<memory>
#include<optional>
#include<deque>

#include "MonitoredProcess.hpp"
#include "fs/ProcessFile.hpp"
#include "fs/ProcessFileSystem.hpp"

namespace twili {
namespace process {

class ECSProcess : public MonitoredProcess {
 public:
	ECSProcess(
		Twili &twili,
		const char *rtld,
		const char *npdm,
		uint64_t title_id);
	
	virtual void ChangeState(MonitoredProcess::State state) override;

	// sets up ExternalContentSource for loader
	// returns false if this process is no longer requested to launch
	virtual bool PrepareForLaunch();
 protected:
	fs::ProcessFileSystem virtual_exefs;

 private:
	uint64_t title_id;
	bool ecs_pending = false;
};

} // namespace process
} // namespace twili
