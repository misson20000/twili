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

#include "AppletProcess.hpp"

#include "../twili.hpp"
#include "fs/ActualFile.hpp"
#include "fs/VectorFile.hpp"

#include "err.hpp"
#include "title_id.hpp"

using namespace trn;

namespace twili {
namespace process {

ShellProcess::ShellProcess(ShellTracker &tracker) :
	TrackedProcess<ShellProcess>(
		tracker.twili,
		tracker,
		"/squash/shell_shim.nso",
		"/squash/shell_shim.npdm",
		title_id::ShellProcessDefaultTitle) {
}

void ShellProcess::KillImpl() {
	// no assert so we don't die if this fails
	twili.services.ns_dev.SendSyncRequest<1>( // TerminateProcess
		trn::ipc::InRaw<uint64_t>(GetPid()));
}

void ShellProcess::AddHBABIEntries(std::vector<loader_config_entry_t> &entries) {
	MonitoredProcess::AddHBABIEntries(entries);
}

} // namespace process
} // namespace twili
