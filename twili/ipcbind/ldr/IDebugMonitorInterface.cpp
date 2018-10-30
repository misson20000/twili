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

#include "IDebugMonitorInterface.hpp"

using namespace trn;

namespace twili {
namespace service {
namespace ldr {

IDebugMonitorInterface::IDebugMonitorInterface(ipc_object_t object) : object(object) {
}

IDebugMonitorInterface::IDebugMonitorInterface(ipc::client::Object &&object) : object(std::move(object)) {
}

Result<std::vector<NsoInfo>> IDebugMonitorInterface::GetNsoInfos(uint64_t pid) {
	std::vector<NsoInfo> nso_info;
	uint32_t num_nso_infos = 16;

	Result<std::nullopt_t> r(std::nullopt);
	do {
		nso_info.resize(num_nso_infos);
		
		r = object.SendSyncRequest<2>( // GetNsoInfos
			trn::ipc::InRaw<uint64_t>(pid),
			trn::ipc::OutRaw<uint32_t>(num_nso_infos),
			trn::ipc::Buffer<NsoInfo, 0xA>(nso_info.data(), nso_info.size() * sizeof(NsoInfo)));
	} while(r && num_nso_infos > nso_info.size());
	nso_info.resize(num_nso_infos);
	
	return r.map([&](auto const &_ignored) { return nso_info; });
}

} // namespace ldr
} // namespace service
} // namespace twili
