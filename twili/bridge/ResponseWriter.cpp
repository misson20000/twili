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

#include "ResponseOpener.hpp"

#include "err.hpp"

namespace twili {
namespace bridge {

using trn::ResultCode;
using trn::ResultError;

ResponseWriter::ResponseWriter(std::shared_ptr<detail::ResponseState> state) : state(state) {
}

void ResponseWriter::Write(uint8_t *data, size_t size) {
	state->SendData(data, size);
}

void ResponseWriter::Write(std::string str) {
	Write((uint8_t*) str.data(), str.size());
}

uint32_t ResponseWriter::Object(std::shared_ptr<bridge::Object> object) {
	size_t index = state->objects.size();
	state->objects.push_back(object);
	return index;
}

void ResponseWriter::Finalize() {
	state->Finalize();
}

} // namespace bridge
} // namespace twili
