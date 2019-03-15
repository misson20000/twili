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
	console(GetStdHandle(STD_INPUT_HANDLE)),
	cb(cb),
	eof_cb(eof_cb) {
}

bool InputPump::WantsSignal() {
	return is_valid;
}

void InputPump::Signal() {
	INPUT_RECORD records[16];
	DWORD num_events;
	if(!ReadConsoleInput(console, records, 16, &num_events)) {
		is_valid = false;
		LogMessage(Warning, "ReadConsoleInput failed: %s", NetErrStr());
		return;
	}
	for(int i = 0; i < num_events; i++) {
		INPUT_RECORD &event = records[i];
		if(event.EventType == KEY_EVENT) {
			KEY_EVENT_RECORD &ker = event.Event.KeyEvent;
			if(ker.bKeyDown) {
				DWORD num_written;
				putc(ker.uChar.AsciiChar, stdout);
				if(ker.wVirtualKeyCode == VK_BACK) {
					buffer.pop_back();
					putc(' ', stdout);
					putc('\b', stdout);
				} else if(ker.wVirtualKeyCode == VK_RETURN) {
					buffer.push_back('\n');
					cb(buffer);
					buffer.clear();
				} else {
					buffer.push_back(ker.uChar.AsciiChar);
				}
			}
		}
	}
}

HANDLE InputPump::GetHandle() {
	return console;
}

} // namespace detail
} // namespace windows
} // namespace platform
} // namespace twili
