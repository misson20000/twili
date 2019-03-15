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

#include "platform/platform.hpp"

#include<optional>

#include "common/Logger.hpp"

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

NetworkError::NetworkError(int en) : std::runtime_error("Failed to retrieve error string") {
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &string, 0, NULL);
}

NetworkError::~NetworkError() {
	LocalFree((LPTSTR) string);
}

const char *NetworkError::what() const noexcept {
	return string;
}

Socket::Socket(int domain, int type, int protocol) : fd(WSASocket(domain, type, protocol, nullptr, 0, 0)) {
	if(fd == INVALID_SOCKET) {
		throw NetworkError(WSAGetLastError());
	}
}

Socket::Socket(SOCKET fd) : fd(fd) {

}

Socket::~Socket() {
	if(fd != INVALID_SOCKET) {
		closesocket(fd);
	}
}

Socket::Socket(Socket &&other) : fd(other.fd) {
	other.fd = INVALID_SOCKET;
}

Socket &Socket::operator=(Socket &&other) {
	if(fd != INVALID_SOCKET) {
		closesocket(fd);
	}
	fd = other.fd;
	other.fd = -1;
	return *this;
}

ssize_t Socket::Recv(void *buffer, size_t length, int flags) {
	DWORD bytes;
	WSABUF buf = { length, (CHAR*) buffer };
	if(WSARecv(fd, &buf, 1, &bytes, 0, nullptr, nullptr) != 0) {
		return -1;
	}
	return bytes;
}

ssize_t Socket::RecvFrom(void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len) {
	DWORD bytes;
	WSABUF buf = { length, (CHAR*) buffer };
	if(WSARecvFrom(fd, &buf, 1, &bytes, 0, address, address_len, nullptr, nullptr) != 0) {
		return -1;
	}
	return bytes;
}

ssize_t Socket::Send(const void *buffer, size_t length, int flags) {
	DWORD bytes;
	WSABUF buf = { length, (CHAR*) buffer };
	if(WSASend(fd, &buf, 1, &bytes, 0, nullptr, nullptr) != 0) {
		return -1;
	}
	return bytes;
}

int Socket::SetSockOpt(int level, int option_name, const void *option_value, socklen_t option_len) {
	return setsockopt(fd, level, option_name, (const char*) option_value, option_len);
}

void Socket::Bind(const struct sockaddr *address, socklen_t address_len) {
	if(bind(fd, address, address_len) == SOCKET_ERROR) {
		throw NetworkError(WSAGetLastError());
	}
}

void Socket::Listen(int backlog) {
	if(listen(fd, backlog) == SOCKET_ERROR) {
		throw NetworkError(WSAGetLastError());
	}
}

Socket Socket::Accept(struct sockaddr *address, socklen_t *address_len) {
	SOCKET fd = WSAAccept(fd, address, address_len, nullptr, NULL); // :thinking: about that NULL
	if(fd == SOCKET_ERROR) {
		throw NetworkError(WSAGetLastError());
	}
	return Socket(fd);
}

void Socket::Connect(const struct sockaddr *address, socklen_t address_len) {
	if(WSAConnect(fd, address, address_len, nullptr, nullptr, nullptr, nullptr) == SOCKET_ERROR) {
		throw NetworkError(WSAGetLastError());
	}
}

void Socket::Close() {
	closesocket(fd);
	fd = INVALID_SOCKET;
}

} // namespace windows
} // namespace platform
} // namespace twili
