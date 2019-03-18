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

#include "Daemon.hpp"

#include "common/config.hpp"
#include "platform/platform.hpp"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#if WITH_SYSTEMD == 1
#include<systemd/sd-daemon.h>
#endif

#include<msgpack11.hpp>
#include<CLI/CLI.hpp>

#if TWIB_NAMED_PIPE_FRONTEND_ENABLED == 1
#include "NamedPipeFrontend.hpp"
#endif

#include "SocketFrontend.hpp"
#include "Protocol.hpp"
#include "err.hpp"

#include <iostream>
#include <ostream>
#include <string>
#include <csignal>

namespace twili {
namespace twib {
namespace daemon {

Daemon::Daemon() :
	local_client(std::make_shared<LocalClient>(*this))
// this comma placement is really gross, but for some reason C++ doesn't seem to allow commas at the end of member initializer lists
#if TWIBD_TCP_BACKEND_ENABLED
	, tcp(*this)
#endif
#if TWIBD_LIBUSB_BACKEND_ENABLED
	, usb(*this)
#endif
#if TWIBD_LIBUSBK_BACKEND_ENABLED
	, usbk(*this)
#endif
	{
	AddClient(local_client);
#if TWIBD_LIBUSB_BACKEND_ENABLED
	usb.Probe();
#endif
#if TWIBD_LIBUSBK_BACKEND_ENABLED
	usbk.Probe();
#endif
}

Daemon::~Daemon() {
	LogMessage(Debug, "destroying twibd");
}

void Daemon::AddDevice(std::shared_ptr<Device> device) {
	std::lock_guard<std::mutex> lock(device_map_mutex);
	LogMessage(Info, "adding device with id %08x", device->device_id);
	std::weak_ptr<Device> &entry = devices[device->device_id];
	std::shared_ptr<Device> entry_lock = entry.lock();
	if(!entry_lock || entry_lock->GetPriority() <= device->GetPriority()) { // don't let tcp devices clobber usb devices
		entry = device;
	}
	
	LogMessage(Debug, "resetting objects on new device");
	local_client->SendRequest(
		Request(nullptr, device->device_id, 0, 0xffffffff, 0)); // we don't care about the response
}

void Daemon::AddClient(std::shared_ptr<Client> client) {
	std::lock_guard<std::mutex> lock(client_map_mutex);

	uint32_t client_id;
	do {
		client_id = rng();
	} while(clients.find(client_id) != clients.end());
	client->client_id = client_id;
	LogMessage(Info, "adding client with newly assigned id %08x", client_id);
	
	clients[client_id] = client;
}

void Daemon::Awaken() {
	dispatch_queue.enqueue(std::monostate {});
}

void Daemon::PostRequest(Request &&request) {
	dispatch_queue.enqueue(request);
}

void Daemon::PostResponse(Response &&response) {
	dispatch_queue.enqueue(response);
}

void Daemon::RemoveClient(std::shared_ptr<Client> client) {
	std::lock_guard<std::mutex> lock(client_map_mutex);
	clients.erase(clients.find(client->client_id));
	LogMessage(Info, "removing client %08x", client->client_id);
}

void Daemon::RemoveDevice(std::shared_ptr<Device> device) {
	std::lock_guard<std::mutex> lock(device_map_mutex);
	devices.erase(devices.find(device->device_id));
	LogMessage(Info, "removing device %08x", device->device_id);
}

void Daemon::Process() {
	std::variant<std::monostate, Request, Response> v;
	LogMessage(Debug, "Process: dequeueing job...");
	dispatch_queue.wait_dequeue(v);
	LogMessage(Debug, "Process: dequeued job: %d", v.index());

	if(v.index() == 0) { // monostate, just a signal to wake thread up
	} else if(v.index() == 1) {
		Request rq = std::get<Request>(v);
		LogMessage(Debug, "dispatching request");
		LogMessage(Debug, "  client id: %08x", rq.client->client_id);
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
			if(rq.command_id == 0xffffffff) {
				LogMessage(Debug, "detected close request for 0x%x", rq.object_id);
				std::shared_ptr<Client> client = rq.client;
				if(client) {
					// disown the object that's being closed
					for(auto i = client->owned_objects.begin(); i != client->owned_objects.end(); ){
						if((*i)->object_id == rq.object_id) {
							// need to mark this so that it doesn't send another close request
							(*i)->valid = false;
							i = client->owned_objects.erase(i);
							LogMessage(Debug, "  disowned from client");
						} else {
							i++;
						}
					}
				} else {
					LogMessage(Warning, "failed to locate client for disownership");
				}
			}
			LogMessage(Debug, "sending request via device");
			device->SendRequest(std::move(rq));
			LogMessage(Debug, "sent request via device");
		}
	} else if(v.index() == 2) {
		Response rs = std::get<Response>(v);
		LogMessage(Debug, "dispatching response");
		LogMessage(Debug, "  client id: %08x", rs.client_id);
		LogMessage(Debug, "  object id: %08x", rs.object_id);
		LogMessage(Debug, "  result code: %08x", rs.result_code);
		LogMessage(Debug, "  tag: %08x", rs.tag);
		LogMessage(Debug, "  objects:");
		for(auto o : rs.objects) {
			LogMessage(Debug, "    0x%x", o->object_id);
		}
		
		std::shared_ptr<Client> client = GetClient(rs.client_id);
		if(!client) {
			LogMessage(Info, "dropping response for bad client: 0x%x", rs.client_id);
			return;
		}
		// add any objects this response included to the client's
		// owned object list, to keep the BridgeObject object alive
		client->owned_objects.insert(
			client->owned_objects.end(),
			rs.objects.begin(),
			rs.objects.end());
		client->PostResponse(rs);
	}
	LogMessage(Debug, "finished process loop");
}

Response Daemon::HandleRequest(Request &rq) {
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
						{"bridge_type", device->GetBridgeType()},
						{"identification", device->identification}
					});
			}

			util::Buffer response_payload;
			
			msgpack11::MsgPack array_pack(device_packs);
			std::string ser = array_pack.dump();
			response_payload.Write<uint64_t>(ser.size());
			response_payload.Write(ser);
			
			r.payload = response_payload.GetData();
			
			return r; }
		case protocol::ITwibMetaInterface::Command::CONNECT_TCP: {
			LogMessage(Debug, "command 1 issued to twibd meta object: CONNECT_TCP");

			util::Buffer buffer(rq.payload);
			uint64_t hostname_len, port_len;
			std::string hostname, port;
			if(!buffer.Read<uint64_t>(hostname_len) ||
				 !buffer.Read(hostname, hostname_len) ||
				 !buffer.Read<uint64_t>(port_len) ||
				 !buffer.Read(port, port_len)) {
				return rq.RespondError(TWILI_ERR_PROTOCOL_BAD_REQUEST);
			}
			LogMessage(Info, "requested to connect to %s:%s", hostname.c_str(), port.c_str());

			Response r = rq.RespondOk();
			util::Buffer response_payload;
			std::string msg = tcp.Connect(hostname, port);
			response_payload.Write<uint64_t>(msg.size());
			response_payload.Write(msg);
			r.payload = response_payload.GetData();
			return r; }
		default:
			return rq.RespondError(TWILI_ERR_PROTOCOL_UNRECOGNIZED_FUNCTION);
		}
	default:
		return rq.RespondError(TWILI_ERR_PROTOCOL_UNRECOGNIZED_OBJECT);
	}
}

std::shared_ptr<Client> Daemon::GetClient(uint32_t client_id) {
	std::shared_ptr<Client> client;
	{
		std::lock_guard<std::mutex> lock(client_map_mutex);
		auto i = clients.find(client_id);
		if(i == clients.end()) {
			LogMessage(Debug, "client id 0x%x is not in map", client_id);
			return std::shared_ptr<Client>();
		}
		client = i->second.lock();
		if(!client) {
			LogMessage(Debug, "client id 0x%x weak pointer expired", client_id);
			return std::shared_ptr<Client>();
		}
		if(client->deletion_flag) {
			LogMessage(Debug, "client id 0x%x deletion flag set", client_id);
			return std::shared_ptr<Client>();
		}
	}
	return client;
}

#if TWIB_TCP_FRONTEND_ENABLED == 1
static std::shared_ptr<frontend::SocketFrontend> CreateTCPFrontend(Daemon &daemon, uint16_t port) {
	struct sockaddr_in6 addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port);
	addr.sin6_addr = in6addr_any;
	return std::make_shared<frontend::SocketFrontend>(daemon, AF_INET6, SOCK_STREAM, (struct sockaddr*) &addr, sizeof(addr));
}
#endif

#if TWIB_UNIX_FRONTEND_ENABLED == 1
static std::shared_ptr<frontend::SocketFrontend> CreateUNIXFrontend(Daemon &daemon, std::string path) {
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path)-1);
	return std::make_shared<frontend::SocketFrontend>(daemon, AF_UNIX, SOCK_STREAM, (struct sockaddr*) &addr, sizeof(addr));
}
#endif

#if TWIB_NAMED_PIPE_FRONTEND_ENABLED == 1
static std::shared_ptr<frontend::NamedPipeFrontend> CreateNamedPipeFrontend(Daemon &daemon) {
	return std::make_shared<frontend::NamedPipeFrontend>(daemon, "foo");
}
#endif

} // namespace daemon
} // namespace twib
} // namespace twili

using namespace twili;
using namespace twili::twib;

daemon::Daemon *g_Daemon;
std::sig_atomic_t g_Running;

extern "C" void sigint_handler(int) {
	g_Running = 0;
	g_Daemon->Awaken();
}

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

	CLI::App app {"Twili debug monitor daemon"};

	int verbosity = 3;
	app.add_flag("-v,--verbose", verbosity, "Enable verbose messages. Use twice to enable debug messages");
	
	bool systemd_mode = false;
#if WITH_SYSTEMD == 1
	app.add_flag("--systemd", systemd_mode, "Log in systemd format and obtain sockets from systemd (disables unix and tcp frontends)");
#endif

#if TWIB_UNIX_FRONTEND_ENABLED == 1
	bool unix_frontend_enabled = true;
	app.add_flag_function(
		"--unix",
		[&unix_frontend_enabled](int count) {
			unix_frontend_enabled = true;
		}, "Enable UNIX socket frontend");
	app.add_flag_function(
		"--no-unix",
		[&unix_frontend_enabled](int count) {
			unix_frontend_enabled = false;
		}, "Disable UNIX socket frontend");
	std::string unix_frontend_path = TWIB_UNIX_FRONTEND_DEFAULT_PATH;
	app.add_option(
		"-P,--unix-path", unix_frontend_path,
		"Path for the twibd UNIX socket frontend")
		->envname("TWIB_UNIX_FRONTEND_PATH");
#endif

#if TWIB_TCP_FRONTEND_ENABLED == 1
	bool tcp_frontend_enabled = true;
	app.add_flag_function(
		"--tcp",
		[&tcp_frontend_enabled](int count) {
			tcp_frontend_enabled = true;
		}, "Enable TCP socket frontend");
	app.add_flag_function(
		"--no-tcp",
		[&tcp_frontend_enabled](int count) {
			tcp_frontend_enabled = false;
		}, "Disable TCP socket frontend");
	uint16_t tcp_frontend_port;
	app.add_option(
		"-p,--tcp-port", tcp_frontend_port,
		"Port for the twibd TCP socket frontend")
		->envname("TWIB_TCP_FRONTEND_PORT");
#endif

#if TWIB_NAMED_PIPE_FRONTEND_ENABLED == 1
	bool named_pipe_frontend_enabled = true;
	app.add_flag_function(
		"--named-pipe",
		[&named_pipe_frontend_enabled](int count) {
			named_pipe_frontend_enabled = true;
		}, "Enable named pipe frontend");
	app.add_flag_function(
		"--no-named-pipe",
		[&named_pipe_frontend_enabled](int count) {
			named_pipe_frontend_enabled = false;
		}, "Disable named pipe frontend");
#endif

	try {
		app.parse(argc, argv);
	} catch(const CLI::ParseError &e) {
		return app.exit(e);
	}

	log::Level min_log_level = log::Level::Message;
	if(verbosity >= 1) {
		min_log_level = log::Level::Info;
	}
	if(verbosity >= 2) {
		min_log_level = log::Level::Debug;
	}
#if WITH_SYSTEMD == 1
	if(systemd_mode) {
		add_log(std::make_shared<log::SystemdLogger>(stderr, min_log_level));
	}
#endif
	if(!systemd_mode) {
		log::init_color();
		log::add_log(std::make_shared<log::PrettyFileLogger>(stdout, min_log_level, log::Level::Error));
		log::add_log(std::make_shared<log::PrettyFileLogger>(stderr, log::Level::Error));
	}

	LogMessage(Message, "starting twibd");
	daemon::Daemon daemon;
	g_Daemon = &daemon;
	g_Running = true;
	
	std::vector<std::shared_ptr<daemon::frontend::Frontend>> frontends;
	if(!systemd_mode) {
#if TWIB_TCP_FRONTEND_ENABLED == 1
		if(tcp_frontend_enabled) {
			frontends.push_back(daemon::CreateTCPFrontend(daemon, tcp_frontend_port));
		}
#endif
#if TWIB_UNIX_FRONTEND_ENABLED == 1
		if(unix_frontend_enabled) {
			frontends.push_back(daemon::CreateUNIXFrontend(daemon, unix_frontend_path));
		}
#endif
#if TWIB_NAMED_PIPE_FRONTEND_ENABLED == 1
		if(named_pipe_frontend_enabled) {
			frontends.push_back(daemon::CreateNamedPipeFrontend(daemon));
		}
#endif
	}

#if WITH_SYSTEMD == 1
	if(systemd_mode) {
		int num_fds = sd_listen_fds(false);
		if(num_fds < 0) {
			LogMessage(Warning, "failed to get FDs from systemd");
		} else {
			LogMessage(Info, "got %d sockets from systemd", num_fds);
			for(int fd = SD_LISTEN_FDS_START; fd < SD_LISTEN_FDS_START + num_fds; fd++) {
				if(sd_is_socket(fd, 0, SOCK_STREAM, 1) == 1) {
					frontends.push_back(std::make_shared<daemon::frontend::SocketFrontend>(daemon, platform::Socket(fd)));
				} else {
					LogMessage(Warning, "got an FD from systemd that wasn't a SOCK_STREAM: %d", fd);
				}
			}
		}
		sd_notify(false, "READY=1");
	}
#endif

	std::signal(SIGINT, &sigint_handler);
	
	while(g_Running) {
		daemon.Process();
	}
	return 0;
}
