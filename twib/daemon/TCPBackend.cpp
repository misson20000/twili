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

#include "platform/platform.hpp"

#include "Daemon.hpp"

namespace twili {
namespace twib {
namespace daemon {
namespace backend {

TCPBackend::TCPBackend(Daemon &daemon) :
	daemon(daemon),
	server_logic(*this),
	event_loop(server_logic),
	listen_member(*this, platform::Socket(AF_INET, SOCK_DGRAM, 0)) {
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(15153);
	listen_member.socket.Bind((sockaddr*) &addr, sizeof(addr));

	ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr("224.0.53.55");
	mreq.imr_interface.s_addr = INADDR_ANY;
	if(listen_member.socket.SetSockOpt(IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*) &mreq, sizeof(mreq)) != 0) {
		LogMessage(Error, "Failed to join multicast group");
		exit(1);
	}

	event_loop.Begin();
}

TCPBackend::~TCPBackend() {
	event_loop.Destroy();
	listen_member.socket.Close();
}

std::string TCPBackend::Connect(std::string hostname, std::string port) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	struct addrinfo *res_raw = 0;
	int err = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &res_raw);
	if(err != 0) {
		return gai_strerror(err);
	}

	auto deleter =
		[](struct addrinfo *r) {
			freeaddrinfo(r);
		};
	std::unique_ptr<addrinfo, decltype(deleter)> res(res_raw, deleter);
	
	try {
		platform::Socket socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		socket.Connect(res->ai_addr, res->ai_addrlen);

		devices.emplace_back(std::make_shared<Device>(std::move(socket), *this))->Begin();
		event_loop.GetNotifier().Notify();
		return "Ok"; 
	} catch(platform::NetworkError &e) {
		return e.what();
	}
}

void TCPBackend::Connect(sockaddr *addr, socklen_t addr_len) {
	if(addr->sa_family == AF_INET) {
		sockaddr_in *addr_in = (sockaddr_in*) addr;
		LogMessage(Info, "  from %s", inet_ntoa(addr_in->sin_addr));
		addr_in->sin_port = htons(15152); // force port number
		
		platform::Socket socket(addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
		socket.Connect(addr, addr_len);

		devices.emplace_back(std::make_shared<Device>(std::move(socket), *this))->Begin();
		LogMessage(Info, "connected to %s", inet_ntoa(addr_in->sin_addr));
		event_loop.GetNotifier().Notify();
	} else {
		LogMessage(Info, "not an IPv4 address");
	}
}

TCPBackend::Device::Device(platform::Socket &&socket, TCPBackend &backend) :
	backend(backend),
	connection(std::move(socket), backend.event_loop.GetNotifier()) {
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
		response_in.objects[i] = std::make_shared<BridgeObject>(backend.daemon, mh.device_id, id);
	}

	// remove from pending requests
	pending_requests.remove_if([this](WeakRequest &r) {
			return r.tag == response_in.tag;
		});
	
	if(response_in.client_id == 0xFFFFFFFF) { // identification meta-client
		Identified(response_in);
	} else {
		backend.daemon.PostResponse(std::move(response_in));
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
	serial_number = obj["serial_number"].string_value();

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

TCPBackend::ListenMember::ListenMember(TCPBackend &backend, platform::Socket &&socket) : platform::EventLoop::SocketMember(std::move(socket)), backend(backend) {
}

bool TCPBackend::ListenMember::WantsRead() {
	return true;
}

void TCPBackend::ListenMember::SignalRead() {
	char buffer[256];
	sockaddr_storage addr_storage;
	sockaddr *addr = (sockaddr*) &addr_storage;
	socklen_t addr_len = sizeof(addr);
	ssize_t r = socket.RecvFrom(buffer, sizeof(buffer)-1, 0, addr, &addr_len);
	LogMessage(Debug, "got 0x%x bytes from listen socket", r);
	if(r < 0) {
		LogMessage(Fatal, "listen socket error: %s", platform::NetErrStr());
		exit(1);
	} else {
		buffer[r] = 0;
		if(!strcmp(buffer, "twili-announce")) {
			LogMessage(Info, "received twili device announcement");
			backend.Connect(addr, addr_len);
		}
	}
}

void TCPBackend::ListenMember::SignalError() {
	LogMessage(Fatal, "listen socket error: %s", platform::NetErrStr());
	exit(1);
}

TCPBackend::ServerLogic::ServerLogic(TCPBackend &backend) : backend(backend) {
}

void TCPBackend::ServerLogic::Prepare(platform::EventLoop &loop) {
	loop.Clear();
	loop.AddMember(backend.listen_member);
	for(auto i = backend.devices.begin(); i != backend.devices.end(); ) {
		common::MessageConnection::Request *rq;
		while((rq = (*i)->connection.Process()) != nullptr) {
			(*i)->IncomingMessage(rq->mh, rq->payload, rq->object_ids);
		}

		if((*i)->connection.error_flag) {
			(*i)->deletion_flag = true;
		}
		
		if((*i)->deletion_flag) {
			if((*i)->added_flag) {
				backend.daemon.RemoveDevice(*i);
			}
			i = backend.devices.erase(i);
			continue;
		} else {
			if((*i)->ready_flag && !(*i)->added_flag) {
				backend.daemon.AddDevice((*i));
				(*i)->added_flag = true;
			}
		}

		loop.AddMember((*i)->connection.member);
		
		i++;
	}
}

} // namespace backend
} // namespace daemon
} // namespace twib
} // namespace twili
