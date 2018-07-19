#include "Twibd.hpp"

#include "platform.hpp"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<libusb.h>
#include<msgpack11.hpp>

#include "SocketFrontend.hpp"
#include "Protocol.hpp"
#include "config.hpp"
#include "err.hpp"

#include <iostream>
#include <ostream>
#include <string>

namespace twili {
namespace twibd {

Twibd::Twibd() : local_client(std::make_shared<LocalClient>(this)), usb(this) {
	AddClient(local_client);
	usb.Probe();
}

Twibd::~Twibd() {
	LogMessage(Debug, "destroying twibd");
}

void Twibd::AddDevice(std::shared_ptr<Device> device) {
	std::lock_guard<std::mutex> lock(device_map_mutex);
	LogMessage(Info, "adding device with id %08x", device->device_id);
	devices[device->device_id] = device;
}

void Twibd::AddClient(std::shared_ptr<Client> client) {
	std::lock_guard<std::mutex> lock(client_map_mutex);

	uint32_t client_id;
	do {
		client_id = rng();
	} while(clients.find(client_id) != clients.end());
	client->client_id = client_id;
	LogMessage(Info, "adding client with newly assigned id %08x", client_id);
	
	clients[client_id] = client;
}

void Twibd::PostRequest(Request &&request) {
	dispatch_queue.enqueue(request);
}

void Twibd::PostResponse(Response &&response) {
	dispatch_queue.enqueue(response);
}

void Twibd::RemoveClient(std::shared_ptr<Client> client) {
	std::lock_guard<std::mutex> lock(client_map_mutex);
	clients.erase(clients.find(client->client_id));
}

void Twibd::RemoveDevice(std::shared_ptr<Device> device) {
	std::lock_guard<std::mutex> lock(device_map_mutex);
	devices.erase(devices.find(device->device_id));
}

void Twibd::Process() {
	std::variant<Request, Response> v;
	dispatch_queue.wait_dequeue(v);

	if(v.index() == 0) {
		Request rq = std::get<Request>(v);
		LogMessage(Debug, "dispatching request");
		LogMessage(Debug, "  client id: %08x", rq.client_id);
		LogMessage(Debug, "  device id: %08x", rq.device_id);
		LogMessage(Debug, "  object id: %08x", rq.object_id);
		LogMessage(Debug, "  command id: %08x", rq.command_id);
		LogMessage(Debug, "  tag: %08x", rq.tag);
		
		if(rq.device_id == 0) {
			PostResponse(HandleRequest(rq));
		} else {
			std::shared_ptr<Device> device;
			{
				std::lock_guard<std::mutex> lock(device_map_mutex);
				auto i = devices.find(rq.device_id);
				if(i == devices.end()) {
					PostResponse(rq.RespondError(TWILI_ERR_PROTOCOL_UNRECOGNIZED_DEVICE));
					return;
				}
				device = i->second.lock();
				if(!device || device->deletion_flag) {
					PostResponse(rq.RespondError(TWILI_ERR_PROTOCOL_UNRECOGNIZED_DEVICE));
					return;
				}
			}
			device->SendRequest(std::move(rq));
		}
	} else if(v.index() == 1) {
		Response rs = std::get<Response>(v);
		LogMessage(Debug, "dispatching response");
		LogMessage(Debug, "  client id: %08x", rs.client_id);
		LogMessage(Debug, "  object id: %08x", rs.object_id);
		LogMessage(Debug, "  result code: %08x", rs.result_code);
		LogMessage(Debug, "  tag: %08x", rs.tag);

		std::shared_ptr<Client> client;
		{
			std::lock_guard<std::mutex> lock(client_map_mutex);
			auto i = clients.find(rs.client_id);
			if(i == clients.end()) {
				LogMessage(Info, "dropping response destined for unknown client %08x", rs.client_id);
				return;
			}
			client = i->second.lock();
			if(!client) {
				LogMessage(Info, "dropping response destined for destroyed client %08x", rs.client_id);
				return;
			}
			if(client->deletion_flag) {
				LogMessage(Info, "dropping response destined for client flagged for deletion %08x", rs.client_id);
				return;
			}
		}
		client->PostResponse(rs);
	}
}

Response Twibd::HandleRequest(Request &rq) {
	switch(rq.object_id) {
	case 0:
		switch((protocol::ITwibMetaInterface::Command) rq.command_id) {
		case protocol::ITwibMetaInterface::Command::LIST_DEVICES: {
			LogMessage(Debug, "command 0 issued to twibd meta object: LIST_DEVICES");
			
			Response r = rq.RespondOk();
			std::vector<msgpack11::MsgPack> device_packs;
			for(auto i = devices.begin(); i != devices.end(); i++) {
				auto device = i->second.lock();
				device_packs.push_back(
					msgpack11::MsgPack::object {
						{"device_id", device->device_id},
						{"identification", device->identification}
					});
			}
			msgpack11::MsgPack array_pack(device_packs);
			std::string ser = array_pack.dump();
			r.payload = std::vector<uint8_t>(ser.begin(), ser.end());
			
			return r; }
		default:
			return rq.RespondError(TWILI_ERR_PROTOCOL_UNRECOGNIZED_FUNCTION);
		}
	default:
		return rq.RespondError(TWILI_ERR_PROTOCOL_UNRECOGNIZED_OBJECT);
	}
}

static std::shared_ptr<frontend::SocketFrontend> CreateTCPFrontend(Twibd &twibd) {
	struct sockaddr_in6 addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons((u_short) 15151);
	addr.sin6_addr = in6addr_any;
	return std::make_shared<frontend::SocketFrontend>(&twibd, AF_INET6, SOCK_STREAM, (struct sockaddr*) &addr, sizeof(addr));
}

#ifndef _WIN32
static std::shared_ptr<frontend::SocketFrontend> CreateUNIXFrontend(Twibd &twibd) {
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, frontend::Twibd_UNIX_SOCKET_PATH, sizeof(addr.sun_path)-1);
	return std::make_shared<frontend::SocketFrontend>(&twibd, AF_UNIX, SOCK_STREAM, (struct sockaddr*) &addr, sizeof(addr));
}
#endif

} // namespace twibd
} // namespace twili

int main(int argc, char *argv[]) {
#ifdef _WIN32
	WSADATA wsaData;
	int err;
	err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0) {
		printf("WSASStartup failed with error: %d\n", err);
		return 1;
	}
#endif

	add_log(std::make_shared<twili::log::PrettyFileLogger>(stdout, twili::log::Level::Debug, twili::log::Level::Error));
	add_log(std::make_shared<twili::log::PrettyFileLogger>(stderr, twili::log::Level::Error));

	LogMessage(Message, "starting twibd");
	twili::twibd::Twibd twibd;
	std::shared_ptr<twili::twibd::frontend::SocketFrontend> tcp_frontend = twili::twibd::CreateTCPFrontend(twibd);
#ifndef WIN32
	std::shared_ptr<twili::twibd::frontend::SocketFrontend unix_frontend> = twili::twibd::CreateUNIXFrontend(twibd);
#endif
	while(1) {
		twibd.Process();
	}
	return 0;
}
