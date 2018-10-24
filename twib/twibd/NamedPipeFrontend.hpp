#pragma once

#include "platform.hpp"

#include<Windows.h>

#include<list>
#include<vector>
#include<thread>
#include<mutex>

#include<stdint.h>

#include "platform/windows.hpp"
#include "Frontend.hpp"
#include "Messages.hpp"
#include "Protocol.hpp"
#include "Buffer.hpp"
#include "NamedPipeMessageConnection.hpp"
#include "NamedPipeServer.hpp"

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
		Client(twibc::NamedPipeServer::Pipe &&pipe, NamedPipeFrontend &frontend);
		~Client();

		virtual void PostResponse(Response &r);

		twibc::NamedPipeMessageConnection connection;
		NamedPipeFrontend &frontend;
		Twibd &twibd;
	};

private:
	Twibd &twibd;

	class Logic : public twibc::NamedPipeServer::Logic {
	public:
		Logic(NamedPipeFrontend &frontend);
		virtual void Prepare(twibc::NamedPipeServer &server) override;
	private:
		NamedPipeFrontend &frontend;
	} pipe_logic;

	class PendingPipe : public twibc::NamedPipeServer::Pipe {
	public:
		PendingPipe(NamedPipeFrontend &frontend);
		void Reset();
		void Connected();
		virtual bool WantsSignalIn() override;
		virtual void SignalIn() override;
	private:
		NamedPipeFrontend & frontend;
	} pending_pipe;

	std::list<std::shared_ptr<Client>> clients;

	twibc::NamedPipeServer pipe_server;
};

} // namespace frontend
} // namespace twibd
} // namespace twili
