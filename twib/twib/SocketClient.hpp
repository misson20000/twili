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

#include "Client.hpp"

#include "Buffer.hpp"
#include "common/SocketMessageConnection.hpp"

namespace twili {
namespace twib {
namespace client {

class SocketClient : public Client {
 public:
	SocketClient(platform::Socket &&socket);
	~SocketClient();
	
 protected:
	virtual void SendRequestImpl(const Request &rq) override;
 private:
	class Logic : public platform::EventLoop::Logic {
	 public:
		Logic(SocketClient &client);
		virtual void Prepare(platform::EventLoop &loop) override;
	 private:
		SocketClient &client;
	} server_logic;
	
	platform::EventLoop event_loop;
	common::SocketMessageConnection connection;
};

} // namespace client
} // namespace twib
} // namespace twili
