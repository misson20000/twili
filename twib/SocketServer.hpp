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
namespace twibc {

class SocketServer {
 public:
	class Logic {
	 public:
		virtual void Prepare(SocketServer &server) = 0;
	};
	
	SocketServer(Logic &logic);
	~SocketServer();

	class Socket {
	 public:
		Socket();
		Socket(SOCKET fd);
		~Socket();

		Socket &operator=(SOCKET fd);

		bool IsValid();
		void Close();
		
		virtual bool WantsRead();
		virtual bool WantsWrite();
		virtual void SignalRead();
		virtual void SignalWrite();
		virtual void SignalError();

#ifdef _WIN32
		platform::windows::Event event;
		size_t last_service = 0;
#endif

		SOCKET fd;
	 private:
		bool is_valid = false;
	};

	void Begin();
	void Destroy();
	void Clear();
	void AddSocket(Socket &socket);

	class EventThreadNotifier : public twibc::EventThreadNotifier {
	 public:
		EventThreadNotifier(SocketServer &server);
		virtual void Notify() const override;
	 private:
		SocketServer &server;
	} const notifier;
	
 private:
	std::vector<std::reference_wrapper<Socket>> sockets;
	Logic &logic;
	
	bool event_thread_destroy = false;
	void event_thread_func();
	std::thread event_thread;
#ifdef _WIN32
	platform::windows::Event notification_event;
#else
	int event_thread_notification_pipe[2];
#endif
};

} // namespace twibc
} // namespace twili
