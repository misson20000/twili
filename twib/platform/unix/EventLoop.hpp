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

#include "platform/unix.hpp"
#include "platform/EventLoop.hpp"

namespace twili {
namespace platform {
namespace unix {
namespace detail {

class EventLoop;

class EventLoopFileMember {
	friend class EventLoop;
 protected:
	virtual bool WantsRead();
	virtual bool WantsWrite();
	virtual void SignalRead();
	virtual void SignalWrite();
	virtual void SignalError();
	virtual File &GetFile() = 0;
 private:
	size_t last_service = 0;
};

// to provide a common interface
class EventLoopSocketMember : public EventLoopFileMember {
 public:
	EventLoopSocketMember(Socket &&socket);

	Socket socket;
 private:
	virtual File &GetFile() final override;
};

class EventLoop : public platform::detail::EventLoopBase<EventLoop, EventLoopFileMember> {
public:
	using FileMember = EventLoopFileMember;
	using SocketMember = EventLoopSocketMember;

	EventLoop(Logic &logic);
	~EventLoop();
	
	virtual const twibc::EventThreadNotifier &GetEventThreadNotifier() override;
protected:
	virtual void event_thread_func() override;

	// TODO: use File to RAII this
	int notification_pipe[2];
	class EventThreadNotifier : public twibc::EventThreadNotifier {
	public:
		EventThreadNotifier(EventLoop &loop);
		virtual void Notify() const override;
	private:
		EventLoop &loop;
	} const notifier;
};

} // namespace detail
} // namespace unix

using EventLoop = unix::detail::EventLoop;

} // namespace platform
} // namespace twili
