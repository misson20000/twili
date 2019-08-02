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

#include<libtransistor/cpp/ipcclient.hpp>

namespace twili {
namespace service {
namespace ldr {

struct NsoInfo {
	union {
		uint8_t build_id[0x20];
		uint64_t build_id_64[4];
	};
	uint64_t addr;
	size_t size;
};

class IDebugMonitorInterface {
 public:
	IDebugMonitorInterface() = default;
	IDebugMonitorInterface(ipc_object_t object);
	IDebugMonitorInterface(trn::ipc::client::Object &&object);

	trn::Result<std::vector<NsoInfo>> GetNsoInfos(uint64_t pid);
	
	trn::ipc::client::Object object;
};

} // namespace ldr
} // namespace service
} // namespace twili
