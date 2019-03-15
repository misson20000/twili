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

#include "InputPump.hpp"

#include "common/Logger.hpp"

namespace twili {
namespace platform {
namespace unix {
namespace detail {

InputPump(
	size_t buffer_size,
	std::function<void(std::vector<uint8_t>&)> cb,
	std::function<void()> eof_cb) :
	file(File(fileno(stdin), false)),
	buffer_size(buffer_size),
	cb(cb),
	eof_cb(eof_cb) {
}

bool InputPump::WantsRead() {
	return true;
}

void InputPump::SignalRead() {
	// fread won't return until it fills the buffer or errors.
	// this is not what we want; we want to send data over the
	// pipe as soon as it comes in.
	buffer.resize(buffer_size);
	ssize_t r = read(file.fd, buffer.data(), buffer.size());
	if(r > 0) {
		buffer.resize(r);
		cb(buffer);
	} else if(r == 0) {
		eof_cb();
	} else {
		throw std::system_error(errno, std::generic_category());
	}
}

File &&InputPump::GetFile() {
	return file;
}

} // namespace detail
} // namespace unix
} // namespace platform
} // namespace twili
