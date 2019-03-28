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

#pragma once

#include "platform/common/fs.hpp"

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
#include<stdint.h>

#include<stdexcept>

namespace twili {
namespace platform {

static inline char *NetErrStr() {
	return strerror(errno);
}

namespace unix {

class File {
public:
	File();
	File(File &&);
	File &operator=(File &&);
	File(const File &) = delete;
	File &operator=(const File &) = delete;

	File(int fd, bool owned=true);
	~File();

	static File OpenForRead(const char *path);
	static File OpenForClobberingWrite(const char *path);
	static File BorrowStdin();
	static File BorrowStdout();

	int fd;
	bool owned;

	int Claim();
	void Close();

	size_t GetSize();
	size_t Read(void *buffer, size_t size);
	size_t Write(const void *buffer, size_t size);
};

class NetworkError : public std::runtime_error {
 public:
	NetworkError(int en);
};

class Socket : public File {
 public:
	Socket(int domain, int type, int protocol);
	Socket(File &&f);
	Socket(Socket &&);
	~Socket();
	
	Socket(const Socket &) = delete;
	Socket &operator=(Socket &&);
	Socket &operator=(const Socket &) = delete;

	ssize_t Recv(void *buf, size_t length, int flags);
	ssize_t RecvFrom(void *buf, size_t length, int flags, struct sockaddr *address, socklen_t *address_len);
	ssize_t Send(const void *buf, size_t length, int flags);
	int SetSockOpt(int level, int option_name, const void *option_value, socklen_t option_len); // no error check
	
	// checks errors for you
	void Bind(const struct sockaddr *address, socklen_t address_len);
	void Listen(int backlog);
	Socket Accept(struct sockaddr *address, socklen_t *address_len);
	void Connect(const struct sockaddr *address, socklen_t address_len);

 private:
	bool should_unlink_unix_socket = false;
	struct sockaddr_un unix_addr;
};

} // namespace unix

using Socket = unix::Socket;
using File = unix::File;
using NetworkError = unix::NetworkError;

} // namespace platform
} // namespace twili
