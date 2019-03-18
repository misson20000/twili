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

#pragma once

#include "platform/common/EventLoop.hpp"

#include<list>
#include<vector>
#include<thread>
#include<mutex>

#include<stdint.h>

namespace twili {
namespace platform {
namespace common {
namespace detail {

template<typename Self, typename Member>
class EventLoopBase {
 public:
	class Notifier {
	 public:
		virtual void Notify() const = 0;
	};

	class Logic {
	public:
		virtual void Prepare(Self &loop) = 0;
	};

	EventLoopBase(Logic &logic) : logic(logic) {
	}
	
	void Begin() {
		event_thread_running = true;
		std::thread event_thread(&EventLoopBase::event_thread_func, this);
		this->event_thread = std::move(event_thread);
	}
	
	// please call this in your destructor
	void Destroy() {
		if(event_thread_running) {
			event_thread_destroy = true;
			GetNotifier().Notify();
			event_thread.join();
			event_thread_running = false;
		}
	}
	
	void Clear() {
		members.clear();
	}
	
	void AddMember(Member &member) {
		members.push_back(member);
	}

	virtual const Notifier &GetNotifier() = 0;
 protected:
	std::vector<std::reference_wrapper<Member>> members;
	Logic &logic;

	bool event_thread_destroy = false;
	bool event_thread_running = false;
	std::thread event_thread;
	virtual void event_thread_func() = 0;
	
	size_t service_timer = 0;
};

} // namespace detail
} // namespace common
} // namespace platform
} // namespace twili
