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

#include<thread>
#include<list>
#include<queue>
#include<mutex>
#include<condition_variable>

#include "common/SocketMessageConnection.hpp"

#include "Buffer.hpp"
#include "Device.hpp"
#include "Messages.hpp"
#include "Protocol.hpp"

namespace twili {
namespace twib {
namespace daemon {

class Twibd;

namespace backend {

class TCPBackend {
 public:
	TCPBackend(Twibd &twibd);
	~TCPBackend();

	std::string Connect(std::string hostname, std::string port);
	void Connect(sockaddr *sockaddr, socklen_t addr_len);
	
	class Device : public daemon::Device, public std::enable_shared_from_this<Device> {
	 public:
		Device(platform::Socket &&socket, TCPBackend &backend);
		~Device();

		void Begin();
		void Identified(Response &r);
		void IncomingMessage(protocol::MessageHeader &mh, util::Buffer &payload, util::Buffer &object_ids);
		virtual void SendRequest(const Request &&r) override;
		virtual int GetPriority() override;
		virtual std::string GetBridgeType() override;
		
		TCPBackend &backend;
		common::SocketMessageConnection connection;
		std::list<WeakRequest> pending_requests;
		Response response_in;
		bool ready_flag = false;
		bool added_flag = false;
	};

 private:
	Twibd &twibd;
	std::list<std::shared_ptr<Device>> devices;

	class ListenMember : public platform::EventLoop::SocketMember {
	 public:
		ListenMember(TCPBackend &backend, platform::Socket &&socket);

		virtual bool WantsRead() override;
		virtual void SignalRead() override;
		virtual void SignalError() override;
	 private:
		TCPBackend &backend;
	} listen_member;

	class ServerLogic : public platform::EventLoop::Logic {
	 public:
		ServerLogic(TCPBackend &backend);
		virtual void Prepare(platform::EventLoop &loop) override;
	 private:
		TCPBackend &backend;
	} server_logic;
	
	platform::EventLoop event_loop;
};

} // namespace backend
} // namespace daemon
} // namespace twib
} // namespace twili
