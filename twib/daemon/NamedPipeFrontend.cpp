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

#include "NamedPipeFrontend.hpp"

#include<algorithm>

#include "Daemon.hpp"

using namespace twili::platform::windows;

namespace twili {
namespace twib {
namespace daemon {
namespace frontend {

NamedPipeFrontend::NamedPipeFrontend(Daemon &daemon, const char *name) : daemon(daemon), pipe_logic(*this), pending_pipe(*this), event_loop(pipe_logic) {
	LogMessage(Debug, "created NamedPipeFrontend");
	event_loop.Begin();
}

NamedPipeFrontend::~NamedPipeFrontend() {
	event_loop.Destroy();
}

NamedPipeFrontend::Client::Client(Pipe &&pipe, NamedPipeFrontend &frontend) :
	connection(std::move(pipe), frontend.event_loop.GetNotifier()),
	frontend(frontend),
	daemon(frontend.daemon) {
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

void NamedPipeFrontend::Logic::Prepare(platform::EventLoop &loop) {
	LogMessage(Debug, "NamedPipeFrontend preparing");
	loop.Clear();
	loop.AddMember(frontend.pending_pipe);

	for(auto i = frontend.clients.begin(); i != frontend.clients.end(); ) {
		common::MessageConnection::Request *rq;
		while((rq = (*i)->connection.Process()) != nullptr) {
			LogMessage(Debug, "posting request");
			frontend.daemon.PostRequest(
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
			frontend.daemon.RemoveClient(*i);
			i = frontend.clients.erase(i);
			continue;
		}

		loop.AddMember((*i)->connection.input_member);
		loop.AddMember((*i)->connection.output_member);
		
		i++;
	}
}

NamedPipeFrontend::PendingPipe::PendingPipe(NamedPipeFrontend &frontend) :
	EventLoopEventMember(),
	frontend(frontend) {
	overlap.hEvent = event.handle;
	Reset();
}

void NamedPipeFrontend::PendingPipe::Reset() {
	pipe = platform::windows::Pipe("\\\\.\\pipe\\twibd",
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
		PIPE_UNLIMITED_INSTANCES, 8192, 8192, 0, nullptr);

	// attempt to connect
	LogMessage(Debug, "PendingPipe attempting to connect...");
	if(ConnectNamedPipe(pipe.handle, &overlap)) {
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
	std::shared_ptr<Client> &client = frontend.clients.emplace_back(std::make_shared<Client>(std::forward<Pipe>(pipe), frontend));
	frontend.daemon.AddClient(client);
	Reset();
}

bool NamedPipeFrontend::PendingPipe::WantsSignal() {
	LogMessage(Debug, "PendingPipe asked if wants signal");
	return true;
}

void NamedPipeFrontend::PendingPipe::Signal() {
	LogMessage(Debug, "PendingPipe signalled");
	DWORD bytes_transferred = 0;
	if(!GetOverlappedResult(pipe.handle, &overlap, &bytes_transferred, false)) {
		LogMessage(Debug, "GetOverlappedResult failed: %d", GetLastError());
		Reset();
	} else {
		Connected();
	}
}

Event &NamedPipeFrontend::PendingPipe::GetEvent() {
	return event;
}

} // namespace frontend
} // namespace daemon
} // namespace twib
} // namespace twili
