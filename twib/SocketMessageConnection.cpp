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

#include "SocketMessageConnection.hpp"

namespace twili {
namespace twibc {

SocketMessageConnection::SocketMessageConnection(SOCKET fd, const EventThreadNotifier &notifier) : socket(*this, fd), notifier(notifier) {
}

SocketMessageConnection::~SocketMessageConnection() {
}

SocketMessageConnection::MessageSocket::MessageSocket(SocketMessageConnection &conn, SOCKET fd) : Socket(fd), connection(conn) {
}

bool SocketMessageConnection::MessageSocket::WantsRead() {
	return true;
}

bool SocketMessageConnection::MessageSocket::WantsWrite() {
	std::lock_guard<std::recursive_mutex> lock(connection.out_buffer_mutex); // ReadAvailable() might not be atomic
	return connection.out_buffer.ReadAvailable() > 0;
}

void SocketMessageConnection::MessageSocket::SignalRead() {
	std::tuple<uint8_t*, size_t> target = connection.in_buffer.Reserve(8192);
	ssize_t r = recv(fd, (char*) std::get<0>(target), std::get<1>(target), 0);
	if(r <= 0) {
		connection.error_flag = true;
	} else {
		connection.in_buffer.MarkWritten(r);
	}
}

void SocketMessageConnection::MessageSocket::SignalWrite() {
	LogMessage(Debug, "pumping out 0x%lx bytes", connection.out_buffer.ReadAvailable());
	std::lock_guard<std::recursive_mutex> lock(connection.out_buffer_mutex);
	if(connection.out_buffer.ReadAvailable() > 0) {
		ssize_t r = send(fd, (char*) connection.out_buffer.Read(), connection.out_buffer.ReadAvailable(), 0);
		if(r < 0) {
			connection.error_flag = true;
			return;
		}
		if(r > 0) {
			connection.out_buffer.MarkRead(r);
		}
	}
}

void SocketMessageConnection::MessageSocket::SignalError() {
	LogMessage(Debug, "error signalled on socket");
	connection.error_flag = true;
}

bool SocketMessageConnection::RequestInput() {
	// unnecessary
	return false;
}

bool SocketMessageConnection::RequestOutput() {
	notifier.Notify();
	return false;
}

} // namespace twibc
} // namespace twili
