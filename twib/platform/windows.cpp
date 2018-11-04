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

#include "windows.hpp"

#include<optional>

#include "Logger.hpp"

namespace twili {
namespace platform {
namespace windows {

KObject::KObject() : handle(INVALID_HANDLE_VALUE) {

}

KObject::KObject(KObject &&other) : handle(other.Claim()) {
}

KObject &KObject::operator=(KObject &&other) {
	Close();
	handle = other.Claim();
	return *this;
}

KObject::KObject(HANDLE hnd) : handle(hnd) {

}

KObject::~KObject() {
	Close();
}

HANDLE KObject::Claim() {
	HANDLE hnd = handle;
	handle = INVALID_HANDLE_VALUE;
	return hnd;
}

void KObject::Close() {
	if(handle != INVALID_HANDLE_VALUE) {
		CloseHandle(handle);
	}
}

Event::Event(SECURITY_ATTRIBUTES *event_attributes, bool manual_reset, bool initial_state, const char *name) : KObject(CreateEventA(event_attributes, manual_reset, initial_state, name)) {
}

Event::Event(HANDLE handle) : KObject(handle) {
}

Event::Event() : Event(nullptr, false, false, nullptr) {
}

Pipe::Pipe(const char *name, uint32_t open_mode, uint32_t pipe_mode, uint32_t max_instances, uint32_t out_buffer_size, uint32_t in_buffer_size, uint32_t default_timeout, SECURITY_ATTRIBUTES *security_attributes)
	: KObject(CreateNamedPipeA(name, open_mode, pipe_mode, max_instances, out_buffer_size, in_buffer_size, default_timeout, security_attributes)) {
	LogMessage(Debug, "tried to create pipe with name '%s'", name);
	if(handle == INVALID_HANDLE_VALUE) {
		LogMessage(Error, "failed to create named pipe: %d", GetLastError());
	}
}

Pipe::Pipe() {

}

Pipe &Pipe::operator=(HANDLE phand) {
	if(handle != INVALID_HANDLE_VALUE) {
		CloseHandle(handle);
	}
	handle = phand;
	return *this;
}

} // namespace windows
} // namespace platform
} // namespace twili