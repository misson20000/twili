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

#include "platform.hpp"
#include "platform/common/EventLoop.hpp"

namespace twili {
namespace platform {
namespace windows {
namespace detail {

class EventLoop;

class EventLoopNativeMember {
	friend class EventLoop;
protected:
	virtual bool WantsSignal();
	virtual void Signal();
	virtual HANDLE GetHandle() = 0;
private:
	size_t last_service = 0;
};

class EventLoopEventMember : public EventLoopNativeMember {
 protected:
	virtual Event &GetEvent() = 0;
 private:
	virtual HANDLE GetHandle() override final;
};

class EventLoopSocketMember : public EventLoopEventMember {
 public:
	EventLoopSocketMember(platform::Socket &&socket);
	platform::Socket socket;
 protected:
	virtual bool WantsRead();
	virtual bool WantsWrite();
	virtual void SignalRead();
	virtual void SignalWrite();
	virtual void SignalError();
 private:
	virtual bool WantsSignal() override final;
	virtual void Signal() override final;
	virtual Event &GetEvent() override final;
	
	platform::windows::Event event;
};

class EventLoop : public platform::common::detail::EventLoopBase<EventLoop, EventLoopNativeMember> {
public:
	using NativeMember = EventLoopNativeMember;
	using EventMember = EventLoopEventMember;
	using SocketMember = EventLoopSocketMember;
	
	EventLoop(Logic &logic);
	~EventLoop();

	virtual Notifier &GetNotifier() override;
protected:
	virtual void event_thread_func() override;

	Event notification_event;
	class EventThreadNotifier : public Notifier {
	public:
		EventThreadNotifier(EventLoop &loop);
		virtual void Notify() const override;
	private:
		EventLoop &loop;
	} notifier;
};

} // namespace detail
} // namespace windows

using EventLoop = windows::detail::EventLoop;

} // namespace platform
} // namespace twili
