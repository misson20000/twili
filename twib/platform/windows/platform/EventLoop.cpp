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

#include "EventLoop.hpp"

#include<algorithm>

#include "common/Logger.hpp"

namespace twili {
namespace platform {
namespace windows {
namespace detail {

EventLoop::EventThreadNotifier::EventThreadNotifier(EventLoop &loop) : loop(loop) {
}

void EventLoop::EventThreadNotifier::Notify() const {
	if(!SetEvent(loop.notification_event.handle)) {
		LogMessage(Fatal, "failed to set notification event");
		exit(1);
	}
}

EventLoop::EventLoop(Logic &logic) :
	platform::common::detail::EventLoopBase<EventLoop, EventLoopEventMember>(logic),
	notifier(*this) {
}

EventLoop::~EventLoop() {
	Destroy();
}

EventLoop::Notifier &EventLoop::GetNotifier() {
	return notifier;
}

void EventLoop::event_thread_func() {
	while(!event_thread_destroy) {
		logic.Prepare(*this);

		std::sort(members.begin(), members.end(), [](auto &a, auto &b) { return a.get().last_service > b.get().last_service; });
		std::vector<HANDLE> event_handles;
		std::vector<std::reference_wrapper<EventMember>> event_members;
		for(auto i = members.begin(); i != members.end(); i++) {
			EventMember &member = i->get();
			if(member.WantsSignal()) {
				event_handles.push_back(member.GetEvent().handle);
				event_members.push_back(member);
			}
		}
		// add notification event last since it's the one we're least interested in
		event_handles.push_back(notification_event.handle);
		
		LogMessage(Debug, "waiting on %d handles", event_handles.size());
		
		DWORD r = WaitForMultipleObjects(event_handles.size(), event_handles.data(), false, INFINITE);
		LogMessage(Debug, "  -> %d", r);
		
		if(r == WAIT_FAILED || r < WAIT_OBJECT_0 || r - WAIT_OBJECT_0 >= event_handles.size()) {
			LogMessage(Fatal, "WaitForMultipleObjects failed");
			exit(1);
		}
		
		if(r == WAIT_OBJECT_0 + event_members.size()) { // notification event
			// go back to logic.Prepare()
			continue;
		}
		
		event_members[r - WAIT_OBJECT_0].get().last_service = 0;
		service_timer++;
		
		for(auto i = event_members.begin() + (r - WAIT_OBJECT_0 + 1); i != event_members.end(); i++) {
			i->get().last_service = service_timer; // increment age on sockets that didn't get a change to signal
		}
		event_members[r - WAIT_OBJECT_0].get().Signal();
	}
}

// default implementations for EventMember
bool EventLoopEventMember::WantsSignal() {
	return false;
}

void EventLoopEventMember::Signal() {
}

EventLoopSocketMember::EventLoopSocketMember(Socket &&socket) : socket(std::move(socket)) {

}

// default implementations for SocketMember
bool EventLoopSocketMember::WantsRead() {
	return false;
}

bool EventLoopSocketMember::WantsWrite() {
	return false;
}

void EventLoopSocketMember::SignalRead() {
	
}

void EventLoopSocketMember::SignalWrite() {
	
}

void EventLoopSocketMember::SignalError() {
	
}

// EventMember implementation for SocketMember
bool EventLoopSocketMember::WantsSignal() {
	return true; // we're always listening for close
}


void EventLoopSocketMember::Signal() {
	WSANETWORKEVENTS netevents;
	if(!WSAEnumNetworkEvents(socket.fd, event.handle, &netevents)) {
		LogMessage(Fatal, "WSAEnumNetworkEvents failed");
		throw NetworkError(WSAGetLastError());
		exit(1);
	}
	for(size_t i = 0; i < netevents.lNetworkEvents; i++) {
		int event = netevents.iErrorCode[i];
		if(event == FD_CLOSE_BIT) {
			SignalError();
		}
		if(event == FD_READ_BIT || event == FD_ACCEPT_BIT) {
			SignalRead();
		}
		if(event == FD_WRITE_BIT) {
			SignalWrite();
		}
	}
}

Event &EventLoopSocketMember::GetEvent() {
	long events = FD_CLOSE;
	if(WantsRead()) {
		events|= FD_READ;
		events|= FD_ACCEPT;
	}
	if(WantsWrite()) {
		events|= FD_WRITE;
	}
	if(WSAEventSelect(socket.fd, event.handle, events)) {
		LogMessage(Fatal, "failed to WSAEventSelect");
		exit(1);
	}
	return event;
}

} // namespace detail
} // namespace windows
} // namespace platform
} // namespace twili
