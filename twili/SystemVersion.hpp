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

#include<compare>

namespace twili {

class SystemVersion {
 public:
	static void SetCurrent();
	static const SystemVersion &Current();
	inline constexpr SystemVersion(uint8_t major, uint8_t minor, uint8_t micro) : major(major), minor(minor), micro(micro) {
	}

	inline constexpr auto operator<=>(const SystemVersion&) const = default;

	const uint8_t major;
	const uint8_t minor;
	const uint8_t micro;
};

}
