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

#include<Windows.h>

#include<list>
#include<vector>
#include<thread>
#include<mutex>

#include<stdint.h>

#include "platform/windows.hpp"
#include "platform/windows/EventLoop.hpp"
#include "Frontend.hpp"
#include "Messages.hpp"
#include "Protocol.hpp"
#include "Buffer.hpp"
#include "NamedPipeMessageConnection.hpp"

namespace twili {
namespace twibd {

class Twibd;

namespace frontend {

class NamedPipeFrontend : public Frontend {
public:
	NamedPipeFrontend(Twibd &twibd, const char *name);
	~NamedPipeFrontend();

	class Client : public twibd::Client {
	public:
		Client(platform::windows::Pipe &&pipe, NamedPipeFrontend &frontend);
		~Client();

		virtual void PostResponse(Response &r);

		twibc::NamedPipeMessageConnection connection;
		NamedPipeFrontend &frontend;
		Twibd &twibd;
	};

private:
	Twibd &twibd;

	class Logic : public platform::windows::EventLoop::Logic {
	public:
		Logic(NamedPipeFrontend &frontend);
		virtual void Prepare(platform::windows::EventLoop &server) override;
	private:
		NamedPipeFrontend &frontend;
	} pipe_logic;

	class PendingPipe : public platform::windows::EventLoop::Member {
	public:
		PendingPipe(NamedPipeFrontend &frontend);
		void Reset();
		void Connected();
		virtual bool WantsSignal() override;
		virtual void Signal() override;
		virtual platform::windows::Event &GetEvent() override;
	private:
		NamedPipeFrontend &frontend;
		platform::windows::Pipe pipe;
		OVERLAPPED overlap = { 0 };
		platform::windows::Event event;
	} pending_pipe;

	std::list<std::shared_ptr<Client>> clients;

	platform::windows::EventLoop event_loop;
};

} // namespace frontend
} // namespace twibd
} // namespace twili
