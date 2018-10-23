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
