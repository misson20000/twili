#include "SocketServer.hpp"

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
	struct sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);
	if(getsockname(fd, (struct sockaddr*) &addr, &addrlen) != 0) {
		LogMessage(Fatal, "failed to get local server address: %s", NetErrStr());
		exit(1);
	}

	// Connect to the server in order to wake it up
	SOCKET cfd = socket(addr.ss_family, socktype, 0);
	if(cfd < 0) {
		LogMessage(Fatal, "failed to create socket: %s", NetErrStr());
		exit(1);
	}

	if(connect(cfd, (struct sockaddr*) &addr, addrlen) < 0) {
		LogMessage(Fatal, "failed to connect for notification: %s", NetErrStr());
		exit(1);
	}
	
	closesocket(cfd);
#else
	char buf[] = ".";
	if(write(server.event_thread_notification_pipe[1], buf, sizeof(buf)) != sizeof(buf)) {
		LogMessage(Fatal, "failed to write to event thread notification pipe: %s", strerror(errno));
		exit(1);
	}
#endif
}

void SocketServer::event_thread_func() {
	fd_set readfds;
	fd_set writefds;
	fd_set errorfds;
	SOCKET max_fd = 0;
	while(!event_thread_destroy) {
		LogMessage(Debug, "socket event thread loop");

		logic.Prepare(*this);
		
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
		
#ifndef _WIN32
		// add event thread notification pipe
		max_fd = std::max(max_fd, event_thread_notification_pipe[0]);
		FD_SET(event_thread_notification_pipe[0], &readfds);
#endif
		
		if(select(max_fd + 1, &readfds, &writefds, &errorfds, NULL) < 0) {
			LogMessage(Fatal, "failed to select file descriptors: %s", NetErrStr());
			exit(1);
		}

#ifndef _WIN32
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
#endif

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
	}
}

} // namespace twibc
} // namespace twili
