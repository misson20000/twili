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

#include "Process.hpp"

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/cpp/ipcclient.hpp>
#include<libtransistor/cpp/ipc/sm.hpp>
#include<libtransistor/util.h>

#include "../twili.hpp"
#include "../Services.hpp"
#include "../ELFCrashReport.hpp"

using namespace trn;

namespace twili {
namespace process {

Process::Process(Twili &twili) : twili(twili) {
}

Process::~Process() {
}

void Process::AddNotes(ELFCrashReport &report) {
	// write nso info notes
	std::vector<hos_types::LoadedModuleInfo> infos;
	twili.services->GetNsoInfos(GetPid(), &infos); // discard result- this is best effort
		
	for(auto &info : infos) {
		ELF::Note::twili_nso_info twinso = {
			.addr = info.addr,
			.size = info.size,
		};
		memcpy(twinso.build_id, info.build_id, sizeof(info.build_id));
		report.AddNote<ELF::Note::twili_nso_info>("Twili", ELF::NT_TWILI_NSO, twinso);
	}
}

} // namespace process
} // namespace twili
