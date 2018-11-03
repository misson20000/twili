#pragma once

#include "Client.hpp"

#include "platform.hpp"
#include "Buffer.hpp"
#include "NamedPipeMessageConnection.hpp"
#include "platform/windows/EventLoop.hpp"

namespace twili {
namespace twib {
namespace client {

class NamedPipeClient : public Client {
public:
	NamedPipeClient(platform::windows::Pipe &&pipe);
	~NamedPipeClient();
protected:
	virtual void SendRequestImpl(const Request &rq) override;
private:
	class Logic : public platform::windows::EventLoop::Logic {
	public:
		Logic(NamedPipeClient &client);
		virtual void Prepare(platform::windows::EventLoop &loop) override;
	private:
		NamedPipeClient &client;
	} pipe_logic;
	platform::windows::EventLoop event_loop;
	twibc::NamedPipeMessageConnection connection;
};

} // namespace client
} // namespace twib
} // namespace twili
