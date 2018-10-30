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

SocketMessageConnection::SocketMessageConnection(SOCKET fd, EventThreadNotifier &notifier) : fd(fd), notifier(notifier) {
}

SocketMessageConnection::~SocketMessageConnection() {
	closesocket(fd);
}

bool SocketMessageConnection::HasOutData() {
	std::lock_guard<std::mutex> lock(out_buffer_mutex); // ReadAvailable() might not be atomic
	return out_buffer.ReadAvailable() > 0;
}

bool SocketMessageConnection::PumpOutput() {
	LogMessage(Debug, "pumping out 0x%lx bytes", out_buffer.ReadAvailable());
	std::lock_guard<std::mutex> lock(out_buffer_mutex);
	if(out_buffer.ReadAvailable() > 0) {
		ssize_t r = send(fd, (char*) out_buffer.Read(), out_buffer.ReadAvailable(), 0);
		if(r < 0) {
			return false;
		}
		if(r > 0) {
			out_buffer.MarkRead(r);
		}
	}
	return true;
}

bool SocketMessageConnection::PumpInput() {
	std::tuple<uint8_t*, size_t> target = in_buffer.Reserve(8192);
	ssize_t r = recv(fd, (char*) std::get<0>(target), std::get<1>(target), 0);
	if(r <= 0) {
		return false;
	} else {
		in_buffer.MarkWritten(r);
	}
	return true;
}

void SocketMessageConnection::SignalInput() {
	// unnecessary
}

void SocketMessageConnection::SignalOutput() {
	notifier.Notify();
}

} // namespace twibc
} // namespace twili
