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

#include "Socket.hpp"

#ifdef __SWITCH__
#include<libtransistor/ipc/bsd.h>
#define closesocket bsd_close
#else
#include "platform.hpp"
#endif

namespace twili {
namespace util {

Socket::Socket(int fd) : fd(fd) {
}

Socket::~Socket() {
	if(fd != -1) {
		closesocket(fd);
	}
}

Socket::Socket(Socket &&other) : fd(other.fd) {
	other.fd = -1;
}

Socket &Socket::operator=(Socket &&other) {
	if(fd != -1) {
		closesocket(fd);
	}
	fd = other.fd;
	other.fd = -1;
	
	return *this;
}

void Socket::Close() {
	if(fd != -1) {
		closesocket(fd);
		fd = -1;
	}
}

} // namespace util
} // namespace twili
