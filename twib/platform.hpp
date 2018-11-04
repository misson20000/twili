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

// cross-platform networking includes and defines

// this needs to be included super early, because
// windows.h is a load of garbage and includes
// winsock.h, which is incompatible with winsock2.h,
// if we don't include winsock2.h first.

#ifdef _WIN32
#ifdef _WINDOWS_
#error windows already included
#endif
#define NOMINMAX
#include<Winsock2.h>
#include<WS2tcpip.h>
#include<io.h>
#include "platform/windows.hpp";

typedef signed long long ssize_t;

static inline char *NetErrStr() {
	char *s = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&s, 0, NULL);
	return s;
}

#else

#include<sys/socket.h>
#include<sys/select.h>
#include<sys/un.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>

typedef int SOCKET;
#define INVALID_SOCKET -1
#define closesocket close

static inline char *NetErrStr() {
	return strerror(errno);
}

#endif
