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

#include<optional>
#include<expected.hpp>

namespace twili {
namespace twib {

struct ResultCode {
 public:
	static tl::expected<std::nullopt_t, ResultCode> ExpectOk(result_t code);
	static void AssertOk(result_t code);
	template<typename T> static T&& AssertOk(tl::expected<T, ResultCode> &&monad);
	
	ResultCode(result_t code);
	inline bool IsOk() {
		return code == RESULT_OK;
	}

	trn_result_description_t Lookup();
	
	result_t code;
};

template<typename T>
using Result = tl::expected<T, ResultCode>;

} // namespace twib
} // namespace twili
