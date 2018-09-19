#include "TCPBridge.hpp"

#include<libtransistor/ipc/bsd.h>

#include<errno.h>
#include<system_error>

#include "twili.hpp"

namespace twili {
namespace bridge {
namespace tcp {

using trn::ResultCode;
using trn::ResultError;

class MutexShim {
 public:
	MutexShim(trn_mutex_t &mutex) : mutex(mutex) {}
	void lock() { trn_mutex_lock(&mutex); }
	bool try_lock() { return trn_mutex_try_lock(&mutex); }
	void unlock() { trn_mutex_unlock(&mutex); }
 private:
	trn_mutex_t &mutex;
};

TCPBridge::TCPBridge(Twili &twili, std::shared_ptr<bridge::Object> object_zero) :
	twili(twili),
	network(twili.services.nifm.CreateRequest(2)),
	object_zero(object_zero) {
	printf("initializing TCPBridge\n");
	ResultCode::AssertOk(bsd_init());

	network_state_event = std::move(std::get<0>(network.GetSystemEventReadableHandles()));
	printf("network event: 0x%x\n", network_state_event.handle);
	
	network_state_wh = twili.event_waiter.Add(
		network_state_event,
		[this]() {
			printf("received network state event notification\n");
			network_state_event.ResetSignal();

			MutexShim mutex_shim(network_state_mutex);
			std::unique_lock<MutexShim> lock(mutex_shim);
			
			network_state = network.GetRequestState();
			printf("network state changed: %d\n", network_state);
			if(network_state != service::nifm::IRequest::State::Connected) {
				server_socket.Close(); // signal event thread if it's blocked in poll
			}
			
			trn_condvar_signal(&network_state_condvar, -1);
			return true;
		});

	network.SetConnectionConfirmationOption(2);
	network.SetPersistent(true);
	network.Submit();
	
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
			MutexShim mutex_shim(network_state_mutex);
			std::unique_lock<MutexShim> lock(mutex_shim);
			if(network_state != service::nifm::IRequest::State::Connected) {
				printf("network is down\n");
				connections.clear(); // kill all our connections
				
				// wait for network to come back up
				printf("waiting for network to come up\n");
				while(network_state != service::nifm::IRequest::State::Connected) {
					trn_condvar_wait(&network_state_condvar, &network_state_mutex, -1);
				}
				printf("network is up\n");

				// recreate server socket
				server_socket = {bsd_socket(AF_INET, SOCK_STREAM, 0)};
				if(server_socket.fd == -1) {
					printf("failed to create socket\n");
					trn_mutex_unlock(&network_state_mutex);
					throw std::system_error(bsd_errno, std::generic_category());
				}
				
				struct sockaddr_in addr;
				memset(&addr, 0, sizeof(addr));
				addr.sin_family = AF_INET;
				addr.sin_port = htons(15152);
				addr.sin_addr = {INADDR_ANY};
				
				if(bsd_bind(server_socket.fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
					printf("failed to bind socket\n");
					trn_mutex_unlock(&network_state_mutex);
					throw std::system_error(bsd_errno, std::generic_category());
				}
				
				if(bsd_listen(server_socket.fd, 20) < 0) {
					printf("failed to listen on socket\n");
					trn_mutex_unlock(&network_state_mutex);
					throw std::system_error(bsd_errno, std::generic_category());
				}
				printf("server socket created\n");
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
			if(network_state == service::nifm::IRequest::State::Connected) {
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
				std::shared_ptr<Connection> connection = std::make_shared<Connection>(*this, std::move(client));
				connections.push_back(connection);
				printf("made connection\n");
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

TCPBridge::~TCPBridge() {
	printf("destroying TCPBridge\n");
	thread_destroy = true;
	server_socket.Close();
	printf("waiting for socket thread to die\n");
	trn_thread_join(&thread, -1);
	printf("socket thread joined\n");
	trn_thread_destroy(&thread);
	bsd_finalize();
}

} // namespace tcp
} // namespace bridge
} // namespace twili
