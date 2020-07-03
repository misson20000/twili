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

#include "TCPBridge.hpp"

#include<libtransistor/ipc/bsd.h>

#include "../Object.hpp"
#include "../ResponseOpener.hpp"

#include "err.hpp"

namespace twili {
namespace bridge {
namespace tcp {

using trn::ResultError;

TCPBridge::Connection::ResponseState::ResponseState(std::shared_ptr<Connection> connection, uint32_t client_id, uint32_t tag) :
	detail::ResponseState(client_id, tag),
	connection(connection) {
	
}

size_t TCPBridge::Connection::ResponseState::GetMaxTransferSize() {
	return 16384;
}

void TCPBridge::Connection::ResponseState::SendHeader(protocol::MessageHeader &hdr) {
	Send((uint8_t*) &hdr, sizeof(hdr));
}

void TCPBridge::Connection::ResponseState::SendData(uint8_t *data, size_t size) {
	Send(data, size);
	transferred_size+= size;
}

void TCPBridge::Connection::ResponseState::Finalize() {
	// these mean that the response code didn't do what it told us it would...
	if(transferred_size != total_size) {
		twili::Abort(TWILI_ERR_BAD_RESPONSE);
	}
	if(objects.size() != object_count) {
		twili::Abort(TWILI_ERR_BAD_RESPONSE);
	}

	if(object_count > 0) {
		std::vector<uint32_t> object_ids;
		for(auto p : objects) {
			object_ids.push_back(p->object_id);
		}
		Send((uint8_t*) object_ids.data(), object_ids.size() * sizeof(uint32_t));
	}
}

uint32_t TCPBridge::Connection::ResponseState::ReserveObjectId() {
	return connection->next_object_id++;
}

void TCPBridge::Connection::ResponseState::InsertObject(std::pair<uint32_t, std::shared_ptr<Object>> &&pair) {
	connection->objects.insert(pair);
}

void TCPBridge::Connection::ResponseState::Send(uint8_t *data, size_t size) {
	while(!connection->deletion_flag && size > 0) {
		ssize_t r = bsd_send(connection->socket.fd, data, size, 0);
		if(r <= 0) {
			connection->deletion_flag = true;
			connection->Panic();
			return;
		}
		size-= r;
		data+= r;
	}
}

} // namespace tcp
} // namespace bridge
} // namespace twili
