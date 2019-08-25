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

#include "platform/common/fs.hpp"

#include<stdint.h>

#include<stdexcept>

// pls
typedef signed long long ssize_t;
#define _fstat fstat

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

	KObject(HANDLE handle, bool owned=true);
	~KObject();

	HANDLE handle;
	bool owned;

	HANDLE Claim();
	void Close();
};

class Event : public KObject {
public:
	Event(SECURITY_ATTRIBUTES *event_attributes, bool manual_reset, bool initial_state, const char *name);
	Event(HANDLE handle);
	Event();

	void Signal();
};

class Pipe : public KObject {
public:
	Pipe(const char *name, uint32_t open_mode, uint32_t pipe_mode, uint32_t max_instances, uint32_t out_buffer_size, uint32_t in_buffer_size, uint32_t default_timeout, SECURITY_ATTRIBUTES *security_attributes);
	Pipe();
	Pipe(HANDLE h);

	static Pipe OpenNamed(const char *path);

	Pipe &operator=(HANDLE handle);
};

class NetworkError : public std::runtime_error {
 public:
	NetworkError(int err);
	~NetworkError();

	virtual const char *what() const noexcept override;
 private:
	const char *string;
};

class Socket {
 public:
	Socket(int domain, int type, int protocol);
	Socket(SOCKET fd);

	Socket(Socket &&);
	Socket &operator=(Socket &&);
	Socket(const Socket&) = delete;
	Socket &operator=(const Socket&) = delete;
	
	~Socket();

	ssize_t Recv(void *buf, size_t length, int flags);
	ssize_t RecvFrom(void *buf, size_t length, int flags, struct sockaddr *address, socklen_t *address_len);
	ssize_t Send(const void *buf, size_t length, int flags);
	int SetSockOpt(int level, int option_name, const void *option_value, socklen_t option_len);

	void Bind(const struct sockaddr *address, socklen_t address_len);
	void Listen(int backlog);
	Socket Accept(struct sockaddr *address, socklen_t *address_len);
	void Connect(const struct sockaddr *address, socklen_t address_len);

	void Close();

	SOCKET fd;
};

class File : public KObject {
public:
	File();
	File(HANDLE handle, bool owned=true);
	static File OpenForRead(const char *path);
	static File OpenForClobberingWrite(const char *path);
	static File BorrowStdin();
	static File BorrowStdout();

	size_t GetSize();
	size_t Read(void *buffer, size_t size);
	size_t Write(const void *buffer, size_t size);
};

} // namespace windows

using Socket = windows::Socket;
using File = windows::File;
using NetworkError = windows::NetworkError;

} // namespace platform
} // namespace twili

#ifndef PLATFORM
#undef CreateFile
#undef DeleteFile
#undef CreateDirectory
#undef SendMessage
#endif
