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

#include "platform.hpp"

namespace twili {
namespace platform {
namespace unix {

File::File() : fd(-1), owned(false) {
}

File::File(File &&other) : fd(other.fd), owned(other.owned) {
	other.owned = false;
}

File &File::operator=(File &&other) {
	if(owned) {
		close(fd);
	}
	owned = other.owned;
	fd = other.fd;
	other.owned = false;
	return *this;
}

File::File(int fd, bool owned) : fd(fd), owned(owned) {
}

File::~File() {
	if(owned) {
		close(fd);
	}
}

int File::Claim() {
	owned = false;
	return fd;
}

void File::Close() {
	if(owned) {
		close(fd);
		owned = false;
	}
}

NetworkError::NetworkError(int en) : std::runtime_error(strerror(en)) {
}

Socket::Socket(int domain, int type, int protocol) : File(socket(domain, type, protocol)) {
	if(fd < 0) {
		throw NetworkError(errno);
	}
}

Socket::Socket(File &&f) : File(std::move(f)) {
}

Socket::Socket(Socket &&o) : File(std::move(o)) {
}

Socket::~Socket() {
	if(should_unlink_unix_socket) {
		unlink(unix_addr.sun_path);
	}
}

Socket &Socket::operator=(Socket &&o) {
	File::operator=(std::move(o));
	return *this;
}

ssize_t Socket::Recv(void *buf, size_t length, int flags) {
	return recv(fd, buf, length, flags);
}

ssize_t Socket::RecvFrom(void *buf, size_t length, int flags, struct sockaddr *addr, socklen_t *len) {
	return recvfrom(fd, buf, length, flags, addr, len);
}

ssize_t Socket::Send(const void *buf, size_t length, int flags) {
	return send(fd, buf, length, flags);
}

int Socket::SetSockOpt(int level, int option_name, const void *option_value, socklen_t option_len) {
	return setsockopt(fd, level, option_name, option_value, option_len);
}

void Socket::Bind(const struct sockaddr *address, socklen_t address_len) {
	if(bind(fd, address, address_len) != 0) {
		int en = errno;
		if(errno == EADDRINUSE && address->sa_family == AF_UNIX) {
			unlink(((struct sockaddr_un*) address)->sun_path);
			if(bind(fd, address, address_len) != 0) {
				throw NetworkError(errno);
			}
		} else {
			throw NetworkError(errno);
		}
	}
	if(should_unlink_unix_socket) {
		unlink(unix_addr.sun_path);
	}
	if(address->sa_family == AF_UNIX) {
		should_unlink_unix_socket = true;
		unix_addr = *(struct sockaddr_un*) address;
	} else {
		should_unlink_unix_socket = false;
	}
}

void Socket::Listen(int backlog) {
	if(listen(fd, backlog) != 0) {
		throw NetworkError(errno);
	}
}

Socket Socket::Accept(struct sockaddr *address, socklen_t *address_len) {
	int r = accept(fd, address, address_len);
	if(r < 0) {
		throw NetworkError(errno);
	}
	return Socket(File(r));
}

void Socket::Connect(const struct sockaddr *address, socklen_t address_len) {
	if(connect(fd, address, address_len) != 0) {
		throw NetworkError(errno);
	}
}

} // namespace unix
} // namespace platform
} // namespace twili
