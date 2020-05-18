//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2020 misson20000 <xenotoad@xenotoad.net>
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

namespace twili {
namespace hos_types {

struct LoadedModuleInfo {
	union {
		uint8_t build_id[0x20];
		uint64_t build_id_64[4];
	};
	uint64_t addr;
	size_t size;
};

struct ResourceLimitInfo {
	size_t current_value;
	size_t limit_value;
};

enum class ShellEventType : uint32_t {
	Exit = 1,
	Launch = 4,
};

struct ShellEventInfo {
	uint64_t pid;
	ShellEventType event;
};

} // namespace hos_types
} // namespace twili
