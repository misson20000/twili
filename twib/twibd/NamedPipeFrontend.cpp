#include "NamedPipeFrontend.hpp"

#include<algorithm>

#include "Twibd.hpp"

using namespace twili::platform::windows;

namespace twili {
namespace twibd {
namespace frontend {

NamedPipeFrontend::NamedPipeFrontend(Twibd &twibd, const char *name) : twibd(twibd), pipe_logic(*this), pending_pipe(*this), pipe_server(pipe_logic) {
	LogMessage(Debug, "created NamedPipeFrontend");
	pipe_server.Begin();
}

NamedPipeFrontend::~NamedPipeFrontend() {
}

NamedPipeFrontend::Client::Client(twibc::NamedPipeServer::Pipe &&pipe, NamedPipeFrontend &frontend) : connection(std::move(pipe), frontend.pipe_server.notifier), frontend(frontend), twibd(frontend.twibd) {
}

NamedPipeFrontend::Client::~Client() {
	LogMessage(Debug, "destroying client 0x%x", client_id);
}

void NamedPipeFrontend::Client::PostResponse(Response &r) {
	protocol::MessageHeader mh;
	mh.device_id = r.device_id;
	mh.object_id = r.object_id;
	mh.result_code = r.result_code;
	mh.tag = r.tag;
	mh.payload_size = r.payload.size();
	mh.object_count = r.objects.size();
	
	std::vector<uint32_t> object_ids(r.objects.size(), 0);
	std::transform(
		r.objects.begin(), r.objects.end(), object_ids.begin(),
		[](auto const &object) {
			return object->object_id;
		});

	connection.SendMessage(mh, r.payload, object_ids);
}

NamedPipeFrontend::Logic::Logic(NamedPipeFrontend &frontend) : frontend(frontend) {

}

void NamedPipeFrontend::Logic::Prepare(twibc::NamedPipeServer &server) {
	LogMessage(Debug, "NamedPipeFrontend preparing");
	server.Clear();
	server.AddPipe(frontend.pending_pipe);

	for(auto i = frontend.clients.begin(); i != frontend.clients.end(); ) {
		twibc::MessageConnection::Request *rq;
		while((rq = (*i)->connection.Process()) != nullptr) {
			LogMessage(Debug, "posting request");
			frontend.twibd.PostRequest(
				Request(
					*i,
					rq->mh.device_id,
					rq->mh.object_id,
					rq->mh.command_id,
					rq->mh.tag,
					std::vector<uint8_t>(rq->payload.Read(), rq->payload.Read() + rq->payload.ReadAvailable())));
			LogMessage(Debug, "posted request");
		}

		if((*i)->connection.error_flag) {
			(*i)->deletion_flag = true;
		}
		
		if((*i)->deletion_flag) {
			frontend.twibd.RemoveClient(*i);
			i = frontend.clients.erase(i);
			continue;
		}

		server.AddPipe((*i)->connection.pipe);
		
		i++;
	}
}

NamedPipeFrontend::PendingPipe::PendingPipe(NamedPipeFrontend &frontend) :
	Pipe(),
	frontend(frontend) {
	Reset();
}

void NamedPipeFrontend::PendingPipe::Reset() {
	Pipe::operator=(platform::windows::Pipe("\\\\.\\pipe\\twibd",
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
		PIPE_UNLIMITED_INSTANCES, 8192, 8192, 0, nullptr));

	// attempt to connect
	LogMessage(Debug, "PendingPipe attempting to connect...");
	if(ConnectNamedPipe(pipe.handle, &overlap_in)) {
		LogMessage(Error, "ConnectNamedPipe failed: %d", GetLastError());
		exit(1);
	}

	switch(GetLastError()) {
	case ERROR_IO_PENDING:
		LogMessage(Debug, "  -> ERROR_IO_PENDING");
		break;
	case ERROR_PIPE_CONNECTED:
		LogMessage(Debug, "  -> ERROR_PIPE_CONNECTED");
		Connected();
		break;
	default:
		LogMessage(Error, "Invalid error code: %d", GetLastError());
		exit(1);
	}
}

void NamedPipeFrontend::PendingPipe::Connected() {
	std::shared_ptr<Client> &client = frontend.clients.emplace_back(std::make_shared<Client>(std::forward<Pipe>(*this), frontend));
	frontend.twibd.AddClient(client);
	Reset();
}

bool NamedPipeFrontend::PendingPipe::WantsSignalIn() {
	LogMessage(Debug, "PendingPipe asked if wants signal");
	return true;
}

void NamedPipeFrontend::PendingPipe::SignalIn() {
	LogMessage(Debug, "PendingPipe signalled");
	DWORD bytes_transferred = 0;
	if(!GetOverlappedResult(pipe.handle, &overlap_in, &bytes_transferred, false)) {
		LogMessage(Debug, "GetOverlappedResult failed: %d", GetLastError());
		Reset();
	} else {
		Connected();
	}
}

} // namespace frontend
} // namespace twibd
} // namespace twili