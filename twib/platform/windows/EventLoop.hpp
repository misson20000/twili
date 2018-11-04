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
