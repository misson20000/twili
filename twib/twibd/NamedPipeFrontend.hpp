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
