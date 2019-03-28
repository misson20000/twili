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
	handle(GetStdHandle(STD_INPUT_HANDLE)),
	buffer_size(buffer_size),
	cb(cb),
	eof_cb(eof_cb),
	thread([this]() { ThreadFunc(); }) {
	if(event.handle == INVALID_HANDLE_VALUE) {
		LogMessage(Error, "invalid event");
		exit(1);
	}
}

InputPump::~InputPump() {
	is_valid = false;
	CancelSynchronousIo(thread.native_handle());
	thread.join();
}

void InputPump::ThreadFunc() {
	std::unique_lock<std::mutex> guard(lock);
	while(is_valid) {
		buffer.resize(buffer_size);

		DWORD bytes_read;
		if(!ReadFile(handle, buffer.data(), buffer.size(), &bytes_read, nullptr)) {
			is_valid = false;
			event.Signal();
		} else {
			buffer.resize(bytes_read);
			data_pending = true;
			event.Signal();
			while(data_pending) {
				cv.wait(guard);
			}
		}
	}
}

bool InputPump::WantsSignal() {
	return is_valid;
}

void InputPump::Signal() {
	std::unique_lock<std::mutex> guard(lock);
	if(data_pending) {
		cb(buffer);
		data_pending = false;
	}
	if(!is_valid) {
		eof_cb();
	}
	cv.notify_all();
}

Event &InputPump::GetEvent() {
	return event;
}

} // namespace detail
} // namespace windows
} // namespace platform
} // namespace twili
