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

#include<errno.h>
#include<mutex>
#include<system_error>

#include "../../twili.hpp"
#include "../../Services.hpp"

namespace twili {
namespace bridge {
namespace tcp {

using trn::ResultCode;
using trn::ResultError;

TCPBridge::TCPBridge(Twili &twili, std::shared_ptr<bridge::Object> object_zero) :
	twili(twili),
	network(ResultCode::AssertOk(twili.services->CreateRequest(2))),
	object_zero(object_zero) {
	printf("initializing TCPBridge\n");
	ResultCode::AssertOk(bsd_init());

	printf("cancel network?\n");
	network.Cancel();
	printf("cancel network.\n");
	
	network_state_event = std::move(std::get<0>(network.GetSystemEventReadableHandles()));
	printf("network event: 0x%x\n", network_state_event.handle);
	
	network_state_wh = twili.event_waiter.Add(
		network_state_event,
		[this]() {
			printf("received network state event notification\n");
			network_state_event.ResetSignal();

			std::unique_lock<thread::Mutex> lock(network_state_mutex);
			
			network_state = network.GetRequestState();
			printf("network state changed: %d\n", network_state);
			if(network_state != nifm::IRequest::State::Connected) {
				// signal event thread if it's blocked in poll
				announce_socket.Close();
				server_socket.Close();
			}

			network_state_condvar.Signal(-1);
			return true;
		});

	network.SetConnectionConfirmationOption(2);
	network.SetPersistent(true);
	network.Submit();

	request_processing_signal_wh = twili.event_waiter.AddSignal(
		[this]() {
			std::unique_lock<thread::Mutex> lock(request_processing_mutex);

			request_processing_signal_wh->ResetSignal();
			try {
				request_processing_connection->Synchronized();
			} catch(ResultError &e) {
				printf("caught 0x%x while processing request\n", e.code.code);
				request_processing_connection->deletion_flag = true;
			}
			request_processing_connection.reset();

			request_processing_condvar.Signal(-1); // resume I/O thread after we unlock

			return true;
		});
	
	ResultCode::AssertOk(trn_thread_create(&thread, TCPBridge::ThreadEntryShim, this, -1, -2, 0x4000, nullptr));
	ResultCode::AssertOk(trn_thread_start(&thread));
}

void TCPBridge::ThreadEntryShim(void *arg) {
	((TCPBridge*) arg)->SocketThread();
}

void TCPBridge::SocketThread() {
	while(!thread_destroy) {
		{ // scope for lock
			// wait for network connection
			std::unique_lock<thread::Mutex> lock(network_state_mutex);
			if(network_state != nifm::IRequest::State::Connected) {
				printf("network is down\n");
				connections.clear(); // kill all our connections
				
				// wait for network to come back up
				printf("waiting for network to come up\n");
				while(network_state != nifm::IRequest::State::Connected && !thread_destroy) {
					network_state_condvar.Wait(network_state_mutex, -1);
				}
				if(thread_destroy) {
					break;
				}
				printf("network is up\n");

				ResetSockets();
			}
		} // end lock scope
		
		std::vector<pollfd> fds;
		fds.push_back({server_socket.fd, POLLIN}); // server socket

		for(auto &c : connections) {
			fds.push_back({c->socket.fd, POLLIN});
		}

		if(bsd_poll(fds.data(), fds.size(), -1) < 0) {
			printf("poll failure\n");
			thread_destroy = 1;
			return;
		}

		if(fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			printf("server socket error\n");
			printf("  revents: 0x%x\n", fds[0].revents);
			printf("  errno: %d\n", bsd_errno);
			if(network_state == nifm::IRequest::State::Connected) {
				printf("network connection is still up\n");
				thread_destroy = 1;
				return;
			} else {
				// Main thread signals to us when the network connection goes down
				// by closing the server socket. We go back to the start of the loop
				// and wait for the network connection to come up again.
				continue;
			}
		}

		if(fds[0].revents & POLLIN) {
			printf("server socket signal\n");
			util::Socket client;
			client.fd = bsd_accept(server_socket.fd, NULL, NULL);
			if(client.fd < 0) {
				printf("failed to accept incoming connection\n");
			} else {
				printf("accepted %d\n", client.fd);
				try {
					std::shared_ptr<Connection> connection = std::make_shared<Connection>(*this, std::move(client));
					connections.push_back(connection);
					printf("made connection\n");
				} catch(std::bad_alloc &bad) {
					printf("out of memory\n");
				} catch(std::runtime_error &e) {
					printf("caught %s\n", e.what());
				}
			}
		}

		size_t fdi = 1;
		for(auto ci = connections.begin(); ci != connections.end(); fdi++) {
			if(fds[fdi].revents & (POLLERR | POLLHUP | POLLNVAL)) {
				(*ci)->deletion_flag = true;
				ci = connections.erase(ci);
				continue;
			}
			if(fds[fdi].revents & POLLIN) {
				(*ci)->PumpInput();
			}
			ci++;
		}

		for(auto i = connections.begin(); i != connections.end(); ) {
			try {
				(*i)->Process();
			} catch(trn::ResultError &e) {
				printf("error 0x%x\n", e.code.code);
				(*i)->deletion_flag = true;
			}
			
			if((*i)->deletion_flag) {
				i = connections.erase(i);
				continue;
			}
			
			i++;
		}
	}
	printf("socket thread exiting\n");
}

void TCPBridge::ResetSockets() {
	// recreate server socket
	server_socket = {bsd_socket(AF_INET, SOCK_STREAM, 0)};
	if(server_socket.fd == -1) {
		printf("failed to create socket\n");
		network_state_mutex.unlock(); // TODO: why did I put this here?
		return;
	}
				
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(twili.config.tcp_bridge_port);
	addr.sin_addr = {INADDR_ANY};
				
	if(bsd_bind(server_socket.fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		printf("failed to bind socket\n");
		return;
	}
				
	if(bsd_listen(server_socket.fd, 20) < 0) {
		printf("failed to listen on socket\n");
		return;
	}

	// recreate announce socket
	announce_socket = {bsd_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)};
	if(announce_socket.fd == -1) {
		printf("failed to create announce socket\n");
		return;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(15153);
	addr.sin_addr = {INADDR_ANY};
	if(bsd_bind(announce_socket.fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		printf("failed to bind announce socket\n");
		return;
	}

	uint8_t group_addr[] = {224, 0, 53, 55};
	addr.sin_addr.s_addr = *(uint32_t*) group_addr;
	char message[] = "twili-announce";
	ssize_t r = bsd_sendto(announce_socket.fd, message, strlen(message), 0, (sockaddr*) &addr, sizeof(addr));
	printf("sendto result: %ld\n", r);
	printf("  bsd_errno: %d\n", bsd_errno);
}

TCPBridge::~TCPBridge() {
	printf("destroying TCPBridge\n");
	thread_destroy = true;
	server_socket.Close();
	network_state_condvar.Signal(-1);
	printf("waiting for socket thread to die\n");
	trn_thread_join(&thread, -1);
	printf("socket thread joined\n");
	trn_thread_destroy(&thread);
	bsd_finalize();
}

} // namespace tcp
} // namespace bridge
} // namespace twili
