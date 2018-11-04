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

#include<Windows.h>

#include<stdint.h>

namespace twili {
namespace platform {
namespace windows {

class KObject {
public:
	KObject();
	KObject(KObject &&);
	KObject &operator=(KObject &&);
	KObject(const KObject &) = delete;
	KObject &operator=(const KObject &) = delete;

	KObject(HANDLE handle);
	~KObject();

	HANDLE handle;

	HANDLE Claim();
	void Close();
};

class Event : public KObject {
public:
	Event(SECURITY_ATTRIBUTES *event_attributes, bool manual_reset, bool initial_state, const char *name);
	Event(HANDLE handle);
	Event();
};

class Pipe : public KObject {
public:
	Pipe(const char *name, uint32_t open_mode, uint32_t pipe_mode, uint32_t max_instances, uint32_t out_buffer_size, uint32_t in_buffer_size, uint32_t default_timeout, SECURITY_ATTRIBUTES *security_attributes);
	Pipe();

	Pipe &operator=(HANDLE handle);
};

} // namespace windows
} // namespace platform
} // namespace twili
