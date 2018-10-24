#pragma once

#include "Client.hpp"

#include "platform.hpp"
#include "Buffer.hpp"
#include "NamedPipeMessageConnection.hpp"

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
	class Logic : public twibc::NamedPipeServer::Logic {
	public:
		Logic(NamedPipeClient &client);
		virtual void Prepare(twibc::NamedPipeServer &server) override;
	private:
		NamedPipeClient &client;
	} pipe_logic;
	twibc::NamedPipeServer pipe_server;
	twibc::NamedPipeMessageConnection connection;
};

} // namespace client
} // namespace twib
} // namespace twili
