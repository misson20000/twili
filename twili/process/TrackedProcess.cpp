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

#include "TrackedProcess.hpp"

#include "../twili.hpp"
#include "fs/ActualFile.hpp"
#include "fs/VectorFile.hpp"
#include "fs/NRONSOTransmutationFile.hpp"

#include "err.hpp"

using namespace trn;

namespace twili {
namespace process {

TrackedProcessBase::TrackedProcessBase(
	Twili &twili,
	const char *rtld,
	const char *npdm,
	uint64_t title_id) :
	ECSProcess(twili, rtld, npdm, title_id) {
}

void TrackedProcessBase::ChangeState(State state)  {
	ECSProcess::ChangeState(state);
	
	if(state == State::Attached) {
		if(run_opener) {
			auto w = run_opener->BeginOk(sizeof(uint64_t));
			w.Write<uint64_t>(GetPid());
			w.Finalize();
			run_opener.reset();
		}

		// find target entry...
		memory_info_t mi = std::get<0>(ResultCode::AssertOk(svc::QueryProcessMemory(*proc, 0)));
		int coderegions_found = 0;
		while((uint64_t) mi.base_addr + mi.size > 0) {
			if(mi.memory_type == 3 && mi.permission == 5) {
				coderegions_found++;
				if(coderegions_found == 2) { // first region is AppletShim, second region is target
					printf("found target entry at 0x%lx\n", (uint64_t) mi.base_addr);
					target_entry = (uint64_t) mi.base_addr;
				}
			}
			mi = std::get<0>(ResultCode::AssertOk(svc::QueryProcessMemory(*proc, (uint64_t) mi.base_addr + mi.size)));
		}
		if(coderegions_found != 2) {
			printf("CODE REGION COUNT MISMATCH (expected 2, counted %d)\n", coderegions_found);
		}
	}
	if(state == State::Exited) {
		if(run_opener) {
			run_opener->BeginError(GetResult()).Finalize();
			run_opener.reset();
		}
	}
}

void TrackedProcessBase::Kill() {
	if(GetState() == State::Created) {
		ChangeState(State::Exited);
		return;
	}

	if(GetState() == State::Started) {
		// we're already been added to tracker queue...
		// just set this flag so tracker can skip over us
		// when we reach the front of the queue.
		exit_pending = true;
		return;
	}
	
	if(GetState() == State::Exited) {
		return; // nothing to do here
	}

	KillImpl();
}

bool TrackedProcessBase::PrepareForLaunch() {
	if(exit_pending) {
		ChangeState(State::Exited);
		return false; // skip over us
	}
	return ECSProcess::PrepareForLaunch();
}

void TrackedProcessBase::KillImpl() {
	// TODO: ask pm:shell politely
}

} // namespace process
} // namespace twili
