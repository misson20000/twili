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

#include "IDebugMonitorInterface.hpp"

using namespace trn;

namespace twili {
namespace service {
namespace ro {

IDebugMonitorInterface::IDebugMonitorInterface(ipc_object_t object) : object(object) {
}

IDebugMonitorInterface::IDebugMonitorInterface(ipc::client::Object &&object) : object(std::move(object)) {
}

Result<std::vector<NroInfo>> IDebugMonitorInterface::GetNroInfos(uint64_t pid) {
	std::vector<NroInfo> nro_info;
	uint32_t num_nro_infos = 16;

	Result<std::nullopt_t> r(std::nullopt);
	do {
		nro_info.resize(num_nro_infos);
		
		r = object.SendSyncRequest<0>( // GetNroInfos
			trn::ipc::InRaw<uint64_t>(pid),
			trn::ipc::OutRaw<uint32_t>(num_nro_infos),
			trn::ipc::Buffer<NroInfo, 0xA>(nro_info.data(), nro_info.size() * sizeof(NroInfo)));
	} while(r && num_nro_infos > nro_info.size());
	nro_info.resize(num_nro_infos);
	
	return r.map([&](auto const &_ignored) { return nro_info; });
}

} // namespace ro
} // namespace service
} // namespace twili
