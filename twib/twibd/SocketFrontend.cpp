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

#include "SocketFrontend.hpp"

#include "platform.hpp"

#include<algorithm>
#include<iostream>

#include<string.h>

#include "Twibd.hpp"
#include "Protocol.hpp"
#include "config.hpp"

namespace twili {
namespace twibd {
namespace frontend {

SocketFrontend::SocketFrontend(Twibd &twibd, int address_family, int socktype, struct sockaddr *bind_addr, size_t bind_addrlen) :
	twibd(twibd),
	server_socket(*this),
	address_family(address_family),
	socktype(socktype),
	bind_addrlen(bind_addrlen),
	server_logic(*this),
	socket_server(server_logic) {

	if(bind_addr == NULL) {
		LogMessage(Fatal, "failed to allocate bind_addr");
		exit(1);
	}
	memcpy((char*) &this->bind_addr, bind_addr, bind_addrlen);
	
	server_socket = socket(address_family, socktype, 0);
	if(!server_socket.IsValid()) {
		LogMessage(Fatal, "failed to create socket: %s", NetErrStr());
		exit(1);
	}
	
	if(address_family == AF_INET6) {
		int ipv6only = 0;
		if(setsockopt(server_socket.fd, IPPROTO_IPV6, IPV6_V6ONLY, (char*) &ipv6only, sizeof(ipv6only)) == -1) {
			LogMessage(Fatal, "failed to make ipv6 server dual stack: %s", NetErrStr());
		}
	}

	UnlinkIfUnix();
	
	if(bind(server_socket.fd, bind_addr, bind_addrlen) < 0) {
		LogMessage(Fatal, "failed to bind socket: %s", NetErrStr());
		server_socket.Close();
		exit(1);
	}

	if(listen(server_socket.fd, 20) < 0) {
		LogMessage(Fatal, "failed to listen on socket: %s", NetErrStr());
		server_socket.Close();
		UnlinkIfUnix();
		exit(1);
	}

	socket_server.Begin();
}

SocketFrontend::SocketFrontend(Twibd &twibd, int fd) :
	twibd(twibd),
	server_socket(*this, fd),
	address_family(0),
	socktype(SOCK_STREAM),
	server_logic(*this),
	socket_server(server_logic) {
	socket_server.Begin();
}

SocketFrontend::~SocketFrontend() {
	socket_server.Destroy();
	server_socket.Close();
	UnlinkIfUnix();
}

void SocketFrontend::UnlinkIfUnix() {
#ifndef WIN32
	if(address_family == AF_UNIX) {
		unlink(((struct sockaddr_un*) &bind_addr)->sun_path);
	}
#endif
}

SocketFrontend::ServerSocket::ServerSocket(SocketFrontend &frontend) : frontend(frontend) {
}

SocketFrontend::ServerSocket::ServerSocket(SocketFrontend &frontend, SOCKET fd) : Socket(fd), frontend(frontend) {
}

SocketFrontend::ServerSocket &SocketFrontend::ServerSocket::operator=(SOCKET fd) {
	twibc::SocketServer::Socket::operator=(fd);
	return *this;
}

bool SocketFrontend::ServerSocket::WantsRead() {
	return true;
}

void SocketFrontend::ServerSocket::SignalRead() {
	LogMessage(Debug, "incoming connection detected");
	
	int client_fd = accept(fd, NULL, NULL);
	if(client_fd < 0) {
		LogMessage(Warning, "failed to accept incoming connection");
	} else {
		std::shared_ptr<Client> c = std::make_shared<Client>(client_fd, frontend);
		frontend.clients.push_back(c);
		frontend.twibd.AddClient(c);
	}
}

void SocketFrontend::ServerSocket::SignalError() {
	LogMessage(Fatal, "error on server socket");
	exit(1);
}

SocketFrontend::ServerLogic::ServerLogic(SocketFrontend &frontend) : frontend(frontend) {
}

void SocketFrontend::ServerLogic::Prepare(twibc::SocketServer &server) {
	server.Clear();
	server.AddSocket(frontend.server_socket);
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

		server.AddSocket((*i)->connection.socket);
		
		i++;
	}
}

SocketFrontend::Client::Client(SOCKET fd, SocketFrontend &frontend) : connection(fd, frontend.socket_server.notifier), frontend(frontend), twibd(frontend.twibd) {
}

SocketFrontend::Client::~Client() {
	LogMessage(Debug, "destroying client 0x%x", client_id);
}

void SocketFrontend::Client::PostResponse(Response &r) {
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

} // namespace frontend
} // namespace twibd
} // namespace twili
