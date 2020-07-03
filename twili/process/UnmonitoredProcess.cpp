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

#include "UnmonitoredProcess.hpp"

#include<libtransistor/cpp/nx.hpp>

#include "../twili.hpp"
#include "../Services.hpp"

using namespace trn;

namespace twili {
namespace process {

UnmonitoredProcess::UnmonitoredProcess(Twili &twili, uint64_t pid) : Process(twili), pid(pid) {
}

uint64_t UnmonitoredProcess::GetPid() {
	return pid;
}

void UnmonitoredProcess::Terminate() {
	// try to terminate a process via pm:shell
	if(twili.services->TerminateProgram(pid) == RESULT_OK) {
		return;
	}

	// try to terminate a process via svcTerminateDebugProcess as a last resort
	KObject debug = twili::Assert(trn::svc::DebugActiveProcess(pid));
	twili::Assert(svcTerminateDebugProcess(debug.handle));
}

} // namespace process
} // namespace twili
