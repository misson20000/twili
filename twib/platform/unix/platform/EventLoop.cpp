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

#include "platform/EventLoop.hpp"

#include<algorithm>

#include "common/Logger.hpp"

namespace twili {
namespace platform {
namespace unix {
namespace detail {

EventLoop::EventThreadNotifier::EventThreadNotifier(EventLoop &loop) : loop(loop) {
}

void EventLoop::EventThreadNotifier::Notify() const {
	char buf[] = ".";
	if(write(loop.notification_pipe[1], buf, sizeof(buf)) != sizeof(buf)) {
		LogMessage(Fatal, "failed to write to event thread notification pipe: %s", strerror(errno));
		exit(1);
	}
}

EventLoop::EventLoop(Logic &logic) :
	platform::common::detail::EventLoopBase<EventLoop, EventLoopFileMember>(logic),
	notifier(*this) {
	if(pipe(notification_pipe) < 0) {
		LogMessage(Fatal, "failed to create pipe for event thread notifications: %s", strerror(errno));
		exit(1);
	}
}

EventLoop::~EventLoop() {
	Destroy();
	close(notification_pipe[0]);
	close(notification_pipe[1]);
}

EventLoop::Notifier &EventLoop::GetNotifier() {
	return notifier;
}

void EventLoop::event_thread_func() {
	while(!event_thread_destroy) {
		logic.Prepare(*this);

		fd_set readfds;
		fd_set writefds;
		fd_set errorfds;
		int max_fd = 0;

		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&errorfds);
		
		for(auto i = members.begin(); i != members.end(); i++) {
			FileMember &member = i->get();
			File &f = member.GetFile();
			if(member.WantsRead()) {
				FD_SET(f.fd, &readfds);
			}
			if(member.WantsWrite()) {
				FD_SET(f.fd, &writefds);
			}
			FD_SET(f.fd, &errorfds);
			max_fd = std::max(max_fd, f.fd);
		}
		
		// add event thread notification pipe
		max_fd = std::max(max_fd, notification_pipe[0]);
		FD_SET(notification_pipe[0], &readfds);
		
		if(select(max_fd + 1, &readfds, &writefds, &errorfds, NULL) < 0) {
			LogMessage(Fatal, "failed to select file descriptors: %s", NetErrStr());
			exit(1);
		}

		// Check select status on event thread notification pipe
		if(FD_ISSET(notification_pipe[0], &readfds)) {
			char buf[64];
			ssize_t r = read(notification_pipe[0], buf, sizeof(buf));
			if(r < 0) {
				LogMessage(Fatal, "failed to read from event thread notification pipe: %s", strerror(errno));
				exit(1);
			}
			LogMessage(Debug, "event thread notified: '%.*s'", r, buf);
		}

		for(auto i = members.begin(); i != members.end(); i++) {
			FileMember &member = i->get();
			File &f = member.GetFile();
			if(FD_ISSET(f.fd, &readfds)) {
				member.SignalRead();
			}
			if(FD_ISSET(f.fd, &writefds)) {
				member.SignalWrite();
			}
			if(FD_ISSET(f.fd, &errorfds)) {
				member.SignalError();
			}
		}
	}
}

// default implementations for EventLoopFileMember
bool EventLoopFileMember::WantsRead() {
	return false;
}

bool EventLoopFileMember::WantsWrite() {
	return false;
}

void EventLoopFileMember::SignalRead() {
}

void EventLoopFileMember::SignalWrite() {
}

void EventLoopFileMember::SignalError() {
}

// EventLoopFileMember implementation for EventLoopSocketMember
EventLoopSocketMember::EventLoopSocketMember(Socket &&socket) : socket(std::move(socket)) {
}

File &EventLoopSocketMember::GetFile() {
	return socket;
}

} // namespace detail
} // namespace unix
} // namespace platform
} // namespace twili
