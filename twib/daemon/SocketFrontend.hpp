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

#include "platform/platform.hpp"

#include<list>
#include<vector>
#include<thread>
#include<mutex>

#include<stdint.h>

#include "common/SocketMessageConnection.hpp"

#include "Frontend.hpp"
#include "Messages.hpp"
#include "Protocol.hpp"
#include "Buffer.hpp"

namespace twili {
namespace twib {
namespace daemon {

class Daemon;

namespace frontend {

class SocketFrontend : public Frontend {
	public:
	SocketFrontend(Daemon &daemon, int address_family, int socktype, struct sockaddr *bind_addr, size_t bind_addrlen);
	SocketFrontend(Daemon &daemon, platform::Socket &&socket);
	~SocketFrontend();

	class Client : public daemon::Client {
		public:
		Client(platform::Socket &&socket, SocketFrontend &frontend);
		~Client();

		virtual void PostResponse(Response &r) override;

		common::SocketMessageConnection connection;
		SocketFrontend &frontend;
		Daemon &daemon;
	};

 private:
	Daemon &daemon;
	class ServerMember : public platform::EventLoop::SocketMember {
	 public:
		ServerMember(SocketFrontend &frontend, platform::Socket &&socket);
		
		virtual bool WantsRead() override;
		virtual void SignalRead() override;
		virtual void SignalError() override;
	 private:
		SocketFrontend &frontend;
	} server_member;

	class ServerLogic : public platform::EventLoop::Logic {
	 public:
		ServerLogic(SocketFrontend &frontend);
		virtual void Prepare(platform::EventLoop &loop) override;
	 private:
		SocketFrontend &frontend;
	} server_logic;

	int address_family;
	int socktype;
	struct sockaddr_storage bind_addr;
	size_t bind_addrlen;
	
	std::list<std::shared_ptr<Client>> clients;
	platform::EventLoop event_loop;
};

} // namespace frontend
} // namespace daemon
} // namespace twib
} // namespace twili
