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
	if(transferred_size != total_size) {
		throw ResultError(TWILI_ERR_BAD_RESPONSE);
	}
	if(objects.size() != object_count) {
		throw ResultError(TWILI_ERR_BAD_RESPONSE);
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
	do {
		ssize_t r = bsd_send(connection->socket.fd, data, size, 0);
		if(r <= 0) {
			connection->deletion_flag = true;
			throw ResultError(TWILI_ERR_TCP_TRANSFER);
		}
		size-= r;
		data+= r;
	} while(size > 0);
}

} // namespace tcp
} // namespace bridge
} // namespace twili
