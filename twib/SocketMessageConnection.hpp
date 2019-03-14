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
#include "platform/EventLoop.hpp"
#include "MessageConnection.hpp"
#include "EventThreadNotifier.hpp"

namespace twili {
namespace twibc {

class SocketMessageConnection : public MessageConnection {
 public:
	SocketMessageConnection(platform::Socket &&socket, const EventThreadNotifier &notifier);
	virtual ~SocketMessageConnection() override;

	class ConnectionMember : public platform::EventLoop::SocketMember {
	 public:
		ConnectionMember(SocketMessageConnection &connection, platform::Socket &&socket);

		virtual bool WantsRead() override;
		virtual bool WantsWrite() override;
		virtual void SignalRead() override;
		virtual void SignalWrite() override;
		virtual void SignalError() override;
	 private:
		SocketMessageConnection &connection;
	} member;

 protected:
	virtual bool RequestInput() override;
	virtual bool RequestOutput() override;
 private:
	const EventThreadNotifier &notifier;
};

} // namespace twibc
} // namespace twili
