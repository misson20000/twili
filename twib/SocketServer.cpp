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

#include "SocketServer.hpp"

#include<algorithm>

#include "Logger.hpp"

namespace twili {
namespace twibc {

SocketServer::SocketServer(Logic &logic) : notifier(*this), logic(logic) {
#ifndef _WIN32
	if(pipe(event_thread_notification_pipe) < 0) {
		LogMessage(Fatal, "failed to create pipe for event thread notifications: %s", NetErrStr());
		exit(1);
	}
#endif // _WIN32
}

SocketServer::~SocketServer() {
	Destroy();
	event_thread.join();

#ifndef _WIN32
	close(event_thread_notification_pipe[0]);
	close(event_thread_notification_pipe[1]);
#endif
}

SocketServer::Socket::Socket() : fd(INVALID_SOCKET), is_valid(false) {
}

SocketServer::Socket::Socket(SOCKET fd) : fd(fd), is_valid(true) {
}

SocketServer::Socket::~Socket() {
	Close();
}

SocketServer::Socket &SocketServer::Socket::operator=(SOCKET fd) {
	Close();
	this->fd = fd;
	this->is_valid = true;
	return *this;
}

bool SocketServer::Socket::IsValid() {
	return is_valid;
}

void SocketServer::Socket::Close() {
	if(IsValid()) {
		closesocket(fd);
	}
	is_valid = false;
}

bool SocketServer::Socket::WantsRead() {
	return false;
}

bool SocketServer::Socket::WantsWrite() {
	return false;
}

void SocketServer::Socket::SignalRead() {
}

void SocketServer::Socket::SignalWrite() {
}

void SocketServer::Socket::SignalError() {
}

void SocketServer::Begin() {
	std::thread event_thread(&SocketServer::event_thread_func, this);
	this->event_thread = std::move(event_thread);
}

void SocketServer::Destroy() {
	event_thread_destroy = true;
	notifier.Notify();
}

void SocketServer::Clear() {
	sockets.clear();
}

void SocketServer::AddSocket(Socket &socket) {
	sockets.push_back(socket);
}

SocketServer::EventThreadNotifier::EventThreadNotifier(SocketServer &server) : server(server) {
}

void SocketServer::EventThreadNotifier::Notify() const {
#ifdef _WIN32
	if(!SetEvent(server.notification_event.handle)) {
		LogMessage(Fatal, "failed to set notification event");
		exit(1);
	}
#else
	char buf[] = ".";
	if(write(server.event_thread_notification_pipe[1], buf, sizeof(buf)) != sizeof(buf)) {
		LogMessage(Fatal, "failed to write to event thread notification pipe: %s", strerror(errno));
		exit(1);
	}
#endif
}

void SocketServer::event_thread_func() {
	while(!event_thread_destroy) {
		LogMessage(Debug, "socket event thread loop");

		logic.Prepare(*this);
		
#ifdef _WIN32
		std::sort(sockets.begin(), sockets.end(), [](Socket &a, Socket &b) { return a.last_service > b.last_service; });
		std::vector<HANDLE> event_handles;
		event_handles.push_back(notification_event.handle);
		for(Socket &socket : sockets) {
			long events = FD_CLOSE;
			if(socket.WantsRead()) {
				events |= FD_READ;
				events |= FD_ACCEPT;
			}
			if(socket.WantsWrite()) {
				events |= FD_WRITE;
			}
			if(WSAEventSelect(socket.fd, socket.event.handle, events)) {
				LogMessage(Fatal, "failed to WSAEventSelect");
				exit(1);
			}
			event_handles.push_back(socket.event.handle);
		}
		DWORD r = WaitForMultipleObjects(event_handles.size(), event_handles.data(), false, INFINITE);
		if(r == WAIT_FAILED || r < WAIT_OBJECT_0 || r - WAIT_OBJECT_0 >= sockets.size() + 1) {
			LogMessage(Fatal, "WaitForMultipleObjects failed");
			exit(1);
		}
		if(r == WAIT_OBJECT_0) { // notification event
			continue;
		}
		sockets[r - WAIT_OBJECT_0 - 1].get().last_service = 0;
		for(auto i = sockets.begin() + (r - WAIT_OBJECT_0 - 1); i != sockets.end(); i++) {
			i->get().last_service++; // increment age on sockets that didn't get a change to signal
		}

		Socket &signalled = sockets[r - WAIT_OBJECT_0 - 1].get();
		WSANETWORKEVENTS netevents;
		if(!WSAEnumNetworkEvents(signalled.fd, signalled.event.handle, &netevents)) {
			LogMessage(Fatal, "WSAEnumNetworkEvents failed");
			exit(1);
		}
		for(size_t i = 0; i < netevents.lNetworkEvents; i++) {
			int event = netevents.iErrorCode[i];
			if(event == FD_CLOSE_BIT) {
				signalled.SignalError();
			}
			if(event == FD_READ_BIT || event == FD_ACCEPT_BIT) {
				signalled.SignalRead();
			}
			if(event == FD_WRITE_BIT) {
				signalled.SignalWrite();
			}
		}
#else
		fd_set readfds;
		fd_set writefds;
		fd_set errorfds;
		SOCKET max_fd = 0;

		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&errorfds);

		for(Socket &socket : sockets) {
			if(socket.WantsRead()) {
				FD_SET(socket.fd, &readfds);
			}
			if(socket.WantsWrite()) {
				FD_SET(socket.fd, &writefds);
			}
			FD_SET(socket.fd, &errorfds);
			max_fd = std::max(max_fd, socket.fd);
		}
		
		// add event thread notification pipe
		max_fd = std::max(max_fd, event_thread_notification_pipe[0]);
		FD_SET(event_thread_notification_pipe[0], &readfds);
		
		if(select(max_fd + 1, &readfds, &writefds, &errorfds, NULL) < 0) {
			LogMessage(Fatal, "failed to select file descriptors: %s", NetErrStr());
			exit(1);
		}

		// Check select status on event thread notification pipe
		if(FD_ISSET(event_thread_notification_pipe[0], &readfds)) {
			char buf[64];
			ssize_t r = read(event_thread_notification_pipe[0], buf, sizeof(buf));
			if(r < 0) {
				LogMessage(Fatal, "failed to read from event thread notification pipe: %s", strerror(errno));
				exit(1);
			}
			LogMessage(Debug, "event thread notified: '%.*s'", r, buf);
		}

		for(Socket &socket : sockets) {
			if(FD_ISSET(socket.fd, &readfds)) {
				socket.SignalRead();
			}
			if(FD_ISSET(socket.fd, &writefds)) {
				socket.SignalWrite();
			}
			if(FD_ISSET(socket.fd, &errorfds)) {
				socket.SignalError();
			}
		}
#endif
	}
}

} // namespace twibc
} // namespace twili
