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

#include "NamedPipeClient.hpp"

#include "err.hpp"

namespace twili {
namespace twib {
namespace tool {
namespace client {

NamedPipeClient::NamedPipeClient(platform::windows::Pipe &&pipe) :
	Client(),
	pipe_logic(*this),
	event_loop(pipe_logic),
	connection(std::move(pipe), event_loop.GetNotifier()) {
	event_loop.Begin();
}

NamedPipeClient::~NamedPipeClient() {
	event_loop.Destroy();
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

void NamedPipeClient::Logic::Prepare(platform::EventLoop &loop) {
	loop.Clear();
	common::MessageConnection::Request *rq;
	while((rq = client.connection.Process()) != nullptr) {
		client.PostResponse(rq->mh, rq->payload, rq->object_ids);
	}
	if(!client.connection.error_flag) {
		loop.AddMember(client.connection.input_member);
		loop.AddMember(client.connection.output_member);
	} else {
		client.FailAllRequests(TWILI_ERR_IO_ERROR);
	}
}

} // namespace client
} // namespace tool
} // namespace twib
} // namespace twili
