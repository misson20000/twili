#include "EventLoop.hpp"

#include<algorithm>

#include "Logger.hpp"

namespace twili {
namespace platform {
namespace windows {

EventLoop::EventLoop(Logic &logic) : notifier(*this), logic(logic) {

}

EventLoop::~EventLoop() {
	Destroy();
	event_thread.join();
}

EventLoop::Member::Member() {

}

EventLoop::Member::~Member() {
}

bool EventLoop::Member::WantsSignal() {
	return false;
}

void EventLoop::Member::Signal() {
}

void EventLoop::Begin() {
	std::thread event_thread(&EventLoop::event_thread_func, this);
	this->event_thread = std::move(event_thread);
}

void EventLoop::Destroy() {
	event_thread_destroy = true;
	notifier.Notify();
}

void EventLoop::Clear() {
	members.clear();
}

void EventLoop::AddMember(Member &member) {
	members.push_back(member);
}

EventLoop::EventThreadNotifier::EventThreadNotifier(EventLoop &loop) : loop(loop) {
}

void EventLoop::EventThreadNotifier::Notify() const {
	if(!SetEvent(loop.notification_event.handle)) {
		LogMessage(Fatal, "failed to set notification event");
		exit(1);
	}
}

void EventLoop::event_thread_func() {
	while(!event_thread_destroy) {
		logic.Prepare(*this);

		std::sort(members.begin(), members.end(), [](Member &a, Member &b) { return a.last_service > b.last_service; });
		std::vector<HANDLE> event_handles;
		std::vector<std::reference_wrapper<Member>> event_members;
		for(auto i = members.begin(); i != members.end(); i++) {
			if(i->get().WantsSignal()) {
				event_handles.push_back(i->get().GetEvent().handle);
				event_members.push_back(*i);
			}
		}
		event_handles.push_back(notification_event.handle);
		LogMessage(Debug, "waiting on %d handles", event_handles.size());
		DWORD r = WaitForMultipleObjects(event_handles.size(), event_handles.data(), false, INFINITE);
		LogMessage(Debug, "  -> %d", r);
		if(r == WAIT_FAILED || r < WAIT_OBJECT_0 || r - WAIT_OBJECT_0 >= event_handles.size()) {
			LogMessage(Fatal, "WaitForMultipleObjects failed");
			exit(1);
		}
		if(r == WAIT_OBJECT_0 + event_members.size()) { // notification event
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

} // namespace windows
} // namespace platform
} // namespace twili
