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

#include "ResultError.hpp"

#include<sstream>

namespace twili {
namespace twib {

ResultError::ResultError(uint32_t code) : std::runtime_error("failed to format result code"), code(code) {
	std::ostringstream ss;
	ss << "ResultError: 0x" << std::hex << code;
	description = ss.str();
}

const char *ResultError::what() const noexcept {
	return description.c_str();
}

} // namespace twib
} // namespace twili
