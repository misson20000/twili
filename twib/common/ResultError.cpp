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

#include "Logger.hpp"

namespace twili {
namespace twib {

ResultError::ResultError(uint32_t code) : std::runtime_error("failed to format result code"), code(code), description(ResultDescription::Lookup(code)) {
}

const char *ResultError::what() const noexcept {
	return description.name;
}

[[noreturn]] void ResultError::Die() {
	int r_module = 2000 + (this->code & 511);
	int r_desc = this->code >> 9;
	
	LogMessage(Fatal, "Caught 0x%x (%04d-%04d, %s): %s", this->code, r_module, r_desc, description.name, description.description);
	if(description.help) {
		LogMessage(Fatal, "  %s", description.help);
	}
	if(description.visibility == ResultVisibility::Internal) {
		LogMessage(Fatal, "  This is an internal error! Please report it upstream so it can be fixed.");
	}
	
	exit(1);
}

} // namespace twib
} // namespace twili
