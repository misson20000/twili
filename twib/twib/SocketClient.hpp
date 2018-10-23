#pragma once

#include "Client.hpp"

#include "Buffer.hpp"
#include "SocketMessageConnection.hpp"

namespace twili {
namespace twib {
namespace client {

class SocketClient : public Client {
 public:
	SocketClient(SOCKET fd);
	~SocketClient();
	
 protected:
	virtual void SendRequestImpl(const Request &rq) override;
 private:
	class Logic : public twibc::SocketServer::Logic {
	 public:
		Logic(SocketClient &client);
		virtual void Prepare(twibc::SocketServer &server) override;
	 private:
		SocketClient &client;
	} server_logic;
	
	twibc::SocketServer socket_server;
	twibc::SocketMessageConnection connection;
};

} // namespace client
} // namespace twib
} // namespace twili
