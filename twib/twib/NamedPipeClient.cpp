#include "NamedPipeClient.hpp"

namespace twili {
namespace twib {
namespace client {

NamedPipeClient::NamedPipeClient(platform::windows::Pipe &&pipe) : Client(), pipe_logic(*this), pipe_server(pipe_logic), connection(std::move(pipe), pipe_server.notifier) {
	pipe_server.Begin();
}

NamedPipeClient::~NamedPipeClient() {
	pipe_server.Destroy();
	connection.pipe.Close();
}

void NamedPipeClient::SendRequestImpl(const Request &rq) {
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

NamedPipeClient::Logic::Logic(NamedPipeClient &client) : client(client) {

}

void NamedPipeClient::Logic::Prepare(twibc::NamedPipeServer &server) {
	server.Clear();
	twibc::MessageConnection::Request *rq;
	while((rq = client.connection.Process()) != nullptr) {
		client.PostResponse(rq->mh, rq->payload, rq->object_ids);
	}
	if(client.connection.error_flag) {
		LogMessage(Fatal, "pipe error");
		exit(1);
	}
	server.AddPipe(client.connection.pipe);
}

} // namespace client
} // namespace twib
} // namespace twili