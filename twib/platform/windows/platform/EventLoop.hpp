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

#include "platform.hpp"

#include<list>
#include<vector>
#include<thread>
#include<mutex>

#include<stdint.h>

#include "platform/windows.hpp"
#include "platform/EventLoop.hpp"

namespace twili {
namespace platform {
namespace windows {
namespace detail {

class EventLoop;

class EventLoopEventMember {
	friend class EventLoop;
 protected:
	virtual bool WantsSignal();
	virtual void Signal();
	virtual Event &GetEvent() = 0;
 private:
	size_t last_service = 0;
};

class EventLoopSocketMember : public EventLoopEventMember {
 protected:
	virtual bool WantsRead();
	virtual bool WantsWrite();
	virtual void SignalRead();
	virtual void SignalWrite();
	virtual void SignalError();
	virtual Socket &GetSocket() = 0;
 private:
	virtual bool WantsSignal() override final;
	virtual void Signal() override final;
	virtual Event &GetEvent() override final = 0;
	
	platform::windows::Event event;
	size_t last_service = 0;
};

class EventLoop : public platform::detail::EventLoopBase<EventLoop, EventMember> {
public:
	using EventMember = EventLoopEventMember;
	using SocketMember = EventLoopSocketMember;
	
	virtual const twibc::EventThreadNotifier &GetEventThreadNotifier() override;
protected:
	virtual void event_thread_func() override;

	Event notification_event;
	class EventThreadNotifier : public twibc::EventThreadNotifier {
	public:
		EventThreadNotifier(EventLoop &loop);
		virtual void Notify() const override;
	private:
		EventLoop &loop;
	} const notifier;
};

} // namespace detail
} // namespace windows

using EventLoop = windows::detail::EventLoop;

} // namespace platform
} // namespace twili
