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

// this needs to be included super early, because
// windows.h is a load of garbage and includes
// winsock.h, which is incompatible with winsock2.h,
// if we don't include winsock2.h first.
//

#ifdef _WINDOWS_
#error windows already included
#endif

#define NOMINMAX
#include<Winsock2.h>
#include<WS2tcpip.h>
#include<io.h>
#include<Windows.h>

#include<stdint.h>

typedef signed long long ssize_t; // pls

namespace twili {
namespace platform {

static inline char *NetErrStr() {
	char *s = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&s, 0, NULL);
	return s;
}

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

class Socket {
 public:
	Socket();
	Socket(Socket &&);
	Socket &operator=(Socket &&);
	Socket(const Socket&) = delete;
	Socket &operator=(const Socket&) = delete;
	
	Socket(SOCKET fd);
	~Socket();

	Socket &operator=(SOCKET fd);

	bool IsValid();
	void Close();

	SOCKET fd;
 private:
	bool is_valid = false;
};

} // namespace windows

using EventType = windows::Event;
using SocketType = windows::Socket;

} // namespace platform
} // namespace twili
