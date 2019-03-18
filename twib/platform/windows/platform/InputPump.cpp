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
namespace windows {
namespace detail {

InputPump::InputPump(
	size_t buffer_size,
	std::function<void(std::vector<uint8_t>&)> cb,
	std::function<void()> eof_cb) :
	hFile(GetStdHandle(STD_INPUT_HANDLE)),
	cb(cb),
	eof_cb(eof_cb),
	buffer_size(buffer_size) {
	overlap.hEvent = event.handle;
	Read();
}

void InputPump::Read() {
	DWORD bytes_read;
	buffer.resize(buffer_size);
	if(ReadFile(hFile, buffer.data(), buffer.size(), &bytes_read, &overlap)) {
		buffer.resize(bytes_read);
		cb(buffer);
		Read();
	} else {
		if(GetLastError() != ERROR_IO_PENDING) {
			eof_cb();
			is_valid = false;
		}
	}
}

bool InputPump::WantsSignal() {
	return is_valid;
}

void InputPump::Signal() {
	DWORD bytes_read = 0;
	if(!GetOverlappedResult(hFile, &overlap, &bytes_read, false)) {
		eof_cb();
		is_valid = false;
	} else {
		buffer.resize(bytes_read);
		cb(buffer);
		Read();
	}
}

Event &InputPump::GetEvent() {
	return event;
}

} // namespace detail
} // namespace windows
} // namespace platform
} // namespace twili
