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

#include "SocketMessageConnection.hpp"

namespace twili {
namespace twib {
namespace common {

SocketMessageConnection::SocketMessageConnection(platform::Socket &&socket, const platform::EventLoop::Notifier &notifier) : member(*this, std::move(socket)), notifier(notifier) {
}

SocketMessageConnection::~SocketMessageConnection() {
}

SocketMessageConnection::ConnectionMember::ConnectionMember(SocketMessageConnection &conn, platform::Socket &&socket) : platform::EventLoop::SocketMember(std::move(socket)), connection(conn) {
}

bool SocketMessageConnection::ConnectionMember::WantsRead() {
	return true;
}

bool SocketMessageConnection::ConnectionMember::WantsWrite() {
	std::lock_guard<Semaphore> lock(connection.out_buffer_sema); // ReadAvailable() might not be atomic
	return connection.out_buffer.ReadAvailable() > 0;
}

void SocketMessageConnection::ConnectionMember::SignalRead() {
	std::tuple<uint8_t*, size_t> target = connection.in_buffer.Reserve(8192);
	ssize_t r = socket.Recv(std::get<0>(target), std::get<1>(target), 0);
	if(r <= 0) {
		connection.error_flag = true;
	} else {
		connection.in_buffer.MarkWritten(r);
	}
}

void SocketMessageConnection::ConnectionMember::SignalWrite() {
	LogMessage(Debug, "pumping out 0x%lx bytes", connection.out_buffer.ReadAvailable());
	std::lock_guard<Semaphore> lock(connection.out_buffer_sema);
	if(connection.out_buffer.ReadAvailable() > 0) {
		ssize_t r = socket.Send(connection.out_buffer.Read(), connection.out_buffer.ReadAvailable(), 0);
		if(r < 0) {
			connection.error_flag = true;
			return;
		}
		if(r > 0) {
			connection.out_buffer.MarkRead(r);
		}
	}
}

void SocketMessageConnection::ConnectionMember::SignalError() {
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

} // namespace common
} // namespace twib
} // namespace twili
