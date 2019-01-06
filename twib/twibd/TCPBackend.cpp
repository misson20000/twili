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

#include "TCPBackend.hpp"

#include "platform.hpp"

#include "Twibd.hpp"

namespace twili {
namespace twibd {
namespace backend {

TCPBackend::TCPBackend(Twibd &twibd) :
	twibd(twibd),
	server_logic(*this),
	socket_server(server_logic),
	listen_socket(*this) {
	listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(listen_socket.fd < 0) {
		LogMessage(Error, "Failed to create listening socket: %s", NetErrStr());
		exit(1);
	}

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(15153);
	if(bind(listen_socket.fd, (sockaddr*) &addr, sizeof(addr)) != 0) {
		LogMessage(Error, "Failed to bind listening socket: %s", NetErrStr());
		exit(1);
	}

	ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr("224.0.53.55");
	mreq.imr_interface.s_addr = INADDR_ANY;
	if(setsockopt(listen_socket.fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*) &mreq, sizeof(mreq)) != 0) {
		LogMessage(Error, "Failed to join multicast group");
		exit(1);
	}

	socket_server.Begin();
}

TCPBackend::~TCPBackend() {
	socket_server.Destroy();
	listen_socket.Close();
}

std::string TCPBackend::Connect(std::string hostname, std::string port) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	struct addrinfo *res = 0;
	int err = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &res);
	if(err != 0) {
		return gai_strerror(err);
	}
	
	int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(fd == -1) {
		return NetErrStr();
	}
	if(connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
		closesocket(fd);
		return NetErrStr();
	}
	freeaddrinfo(res);

	devices.emplace_back(std::make_shared<Device>(fd, *this))->Begin();
	socket_server.notifier.Notify();
	return "Ok";
}

void TCPBackend::Connect(sockaddr *addr, socklen_t addr_len) {
	if(addr->sa_family == AF_INET) {
		sockaddr_in *addr_in = (sockaddr_in*) addr;
		LogMessage(Info, "  from %s", inet_ntoa(addr_in->sin_addr));
		addr_in->sin_port = htons(15152); // force port number
		
		int fd = socket(addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
		if(fd == -1) {
			LogMessage(Error, "could not create socket: %s", NetErrStr());
			return;
		}

		if(connect(fd, addr, addr_len) == -1) {
			LogMessage(Error, "could not connect: %s", NetErrStr());
			closesocket(fd);
			return;
		}

		devices.emplace_back(std::make_shared<Device>(fd, *this))->Begin();
		LogMessage(Info, "connected to %s", inet_ntoa(addr_in->sin_addr));
		socket_server.notifier.Notify();
	} else {
		LogMessage(Info, "not an IPv4 address");
	}
}

TCPBackend::Device::Device(SOCKET fd, TCPBackend &backend) :
	backend(backend),
	connection(fd, backend.socket_server.notifier) {
}

TCPBackend::Device::~Device() {
}

void TCPBackend::Device::Begin() {
	SendRequest(Request(std::shared_ptr<Client>(), 0x0, 0x0, (uint32_t) protocol::ITwibDeviceInterface::Command::IDENTIFY, 0xFFFFFFFF, std::vector<uint8_t>()));
}

void TCPBackend::Device::IncomingMessage(protocol::MessageHeader &mh, util::Buffer &payload, util::Buffer &object_ids) {
	response_in.device_id = device_id;
	response_in.client_id = mh.client_id;
	response_in.object_id = mh.object_id;
	response_in.result_code = mh.result_code;
	response_in.tag = mh.tag;
	response_in.payload = std::vector<uint8_t>(payload.Read(), payload.Read() + payload.ReadAvailable());
	
	// create BridgeObjects
	response_in.objects.resize(mh.object_count);
	for(uint32_t i = 0; i < mh.object_count; i++) {
		uint32_t id;
		if(!object_ids.Read(id)) {
			LogMessage(Error, "not enough object IDs");
			return;
		}
		response_in.objects[i] = std::make_shared<BridgeObject>(backend.twibd, mh.device_id, id);
	}

	// remove from pending requests
	pending_requests.remove_if([this](WeakRequest &r) {
			return r.tag == response_in.tag;
		});
	
	if(response_in.client_id == 0xFFFFFFFF) { // identification meta-client
		Identified(response_in);
	} else {
		backend.twibd.PostResponse(std::move(response_in));
	}
}

void TCPBackend::Device::Identified(Response &r) {
	LogMessage(Debug, "got identification response back");
	LogMessage(Debug, "payload size: 0x%x", r.payload.size());
	if(r.result_code != 0) {
		LogMessage(Warning, "device identification error: 0x%x", r.result_code);
		deletion_flag = true;
		return;
	}
	std::string err;
	msgpack11::MsgPack obj = msgpack11::MsgPack::parse(std::string(r.payload.begin() + 8, r.payload.end()), err);
	identification = obj;
	device_nickname = obj["device_nickname"].string_value();
	std::vector<uint8_t> sn = obj["serial_number"].binary_items();
	serial_number = std::string(sn.begin(), sn.end());

	LogMessage(Info, "nickname: %s", device_nickname.c_str());
	LogMessage(Info, "serial number: %s", serial_number.c_str());
	
	device_id = std::hash<std::string>()(serial_number);
	LogMessage(Info, "assigned device id: %08x", device_id);
	ready_flag = true;
}

void TCPBackend::Device::SendRequest(const Request &&r) {
	protocol::MessageHeader mhdr;
	mhdr.client_id = r.client ? r.client->client_id : 0xffffffff;
	mhdr.object_id = r.object_id;
	mhdr.command_id = r.command_id;
	mhdr.tag = r.tag;
	mhdr.payload_size = r.payload.size();
	mhdr.object_count = 0;

	pending_requests.push_back(r.Weak());

	/* TODO: request objects
	std::vector<uint32_t> object_ids(r.objects.size(), 0);
	std::transform(
		r.objects.begin(), r.objects.end(), object_ids.begin(),
		[](auto const &object) {
			return object->object_id;
		});
	connection.out_buffer.Write(object_ids); */
	connection.SendMessage(mhdr, r.payload, std::vector<uint32_t>());
}

int TCPBackend::Device::GetPriority() {
	return 1; // lower priority than USB devices
}

std::string TCPBackend::Device::GetBridgeType() {
	return "tcp";
}

TCPBackend::ListenSocket::ListenSocket(TCPBackend &backend) : backend(backend) {
}

TCPBackend::ListenSocket::ListenSocket(TCPBackend &backend, SOCKET fd) : Socket(fd), backend(backend) {
}

TCPBackend::ListenSocket &TCPBackend::ListenSocket::operator=(SOCKET fd) {
	twibc::SocketServer::Socket::operator=(fd);
	return *this;
}

bool TCPBackend::ListenSocket::WantsRead() {
	return true;
}

void TCPBackend::ListenSocket::SignalRead() {
	char buffer[256];
	sockaddr_storage addr_storage;
	sockaddr *addr = (sockaddr*) &addr_storage;
	socklen_t addr_len = sizeof(addr);
	ssize_t r = recvfrom(backend.listen_socket.fd, buffer, sizeof(buffer)-1, 0, addr, &addr_len);
	LogMessage(Debug, "got 0x%x bytes from listen socket", r);
	if(r < 0) {
		LogMessage(Fatal, "listen socket error: %s", NetErrStr());
		exit(1);
	} else {
		buffer[r] = 0;
		if(!strcmp(buffer, "twili-announce")) {
			LogMessage(Info, "received twili device announcement");
			backend.Connect(addr, addr_len);
		}
	}
}

void TCPBackend::ListenSocket::SignalError() {
	LogMessage(Fatal, "listen socket error: %s", NetErrStr());
	exit(1);
}

TCPBackend::ServerLogic::ServerLogic(TCPBackend &backend) : backend(backend) {
}

void TCPBackend::ServerLogic::Prepare(twibc::SocketServer &server) {
	server.Clear();
	server.AddSocket(backend.listen_socket);
	for(auto i = backend.devices.begin(); i != backend.devices.end(); ) {
		twibc::MessageConnection::Request *rq;
		while((rq = (*i)->connection.Process()) != nullptr) {
			(*i)->IncomingMessage(rq->mh, rq->payload, rq->object_ids);
		}

		if((*i)->connection.error_flag) {
			(*i)->deletion_flag = true;
		}
		
		if((*i)->deletion_flag) {
			if((*i)->added_flag) {
				backend.twibd.RemoveDevice(*i);
			}
			i = backend.devices.erase(i);
			continue;
		} else {
			if((*i)->ready_flag && !(*i)->added_flag) {
				backend.twibd.AddDevice((*i));
				(*i)->added_flag = true;
			}
		}

		server.AddSocket((*i)->connection.socket);
		
		i++;
	}
}

} // namespace backend
} // namespace twibd
} // namespace twili
