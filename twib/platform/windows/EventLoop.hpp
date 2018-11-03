#pragma once

#include "platform.hpp"

#include<list>
#include<vector>
#include<thread>
#include<mutex>

#include<stdint.h>

#include "EventThreadNotifier.hpp"

namespace twili {
namespace platform {
namespace windows {

class EventLoop {
public:
	class Logic {
	public:
		virtual void Prepare(EventLoop &loop) = 0;
	};

	EventLoop(Logic &logic);
	~EventLoop();

	class Member {
	public:
		Member();
		~Member();

		virtual bool WantsSignal();
		virtual void Signal();
		virtual Event &GetEvent() = 0;

		size_t last_service = 0;
	};

	void Begin();
	void Destroy();
	void Clear();
	void AddMember(Member &event);

	class EventThreadNotifier : public twibc::EventThreadNotifier {
	public:
		EventThreadNotifier(EventLoop &loop);
		virtual void Notify() const override;
	private:
		EventLoop &loop;
	} const notifier;

private:
	std::vector<std::reference_wrapper<Member>> members;
	Logic &logic;

	bool event_thread_destroy = false;
	void event_thread_func();
	std::thread event_thread;

	Event notification_event;
	size_t service_timer = 0;
};

} // namespace windows
} // namespace platform
} // namespace twili
