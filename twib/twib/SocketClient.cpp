#include "SocketClient.hpp"

namespace twili {
namespace twib {
namespace client {

SocketClient::SocketClient(SOCKET fd) : server_logic(*this), socket_server(server_logic), connection(fd, socket_server.notifier) {
	socket_server.Begin();
}

SocketClient::~SocketClient() {
	socket_server.Destroy();
	connection.socket.Close();
}

void SocketClient::SendRequestImpl(const Request &rq) {
	protocol::MessageHeader mh;
	mh.device_id = rq.device_id;
	mh.object_id = rq.object_id;
	mh.command_id = rq.command_id;
	mh.tag = rq.tag;
	mh.payload_size = rq.payload.size();
	mh.object_count = 0;

	connection.SendMessage(mh, rq.payload, std::vector<uint32_t>());
	LogMessage(Debug, "sent request");
}

SocketClient::Logic::Logic(SocketClient &client) : client(client) {
}

void SocketClient::Logic::Prepare(twibc::SocketServer &server) {
	server.Clear();
	twibc::SocketMessageConnection::Request *rq;
	while((rq = client.connection.Process()) != nullptr) {
		client.PostResponse(rq->mh, rq->payload, rq->object_ids);
	}
	if(client.connection.error_flag) {
		LogMessage(Fatal, "socket error");
		exit(1);
	}
	server.AddSocket(client.connection.socket);
}

} // namespace client
} // namespace twib
} // namespace twili
