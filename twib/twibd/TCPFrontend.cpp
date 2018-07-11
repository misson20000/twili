#ifdef _WIN32
#include<ws2tcpip.h>
#include<winsock2.h>
#else
#include<sys/socket.h>
#include<sys/select.h>
#include<netinet/in.h>
#endif

#include "TCPFrontend.hpp"

#include<algorithm>

#include<string.h>
#include<unistd.h>
#include<iostream>

#include "Twibd.hpp"
#include "Protocol.hpp"
#include "config.hpp"

namespace twili {
namespace twibd {
namespace frontend {

#ifdef _WIN32
char *GetFormattedMessage() {
	char *s = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&s, 0, NULL);
	return s;
}

#define NET_ERR_STR GetFormattedMessage()
#else
#define NET_ERR_STR strerror(errno)
#endif

TCPFrontend::TCPFrontend(Twibd *twibd) : twibd(twibd) {
	fd = socket(AF_INET6, SOCK_STREAM, 0);
	if (fd < 0) {
		log(FATAL, "failed to create TCP socket: %s", NET_ERR_STR);
		exit(1);
	}
	log(DEBUG, "created TCP socket: %d", fd);
	int ipv6only = 0;
	if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6only, sizeof(ipv6only)) == -1) {
		log(FATAL, "Failed to make ipv6 server dual stack: %s", NET_ERR_STR);
	}

	struct sockaddr_in6 addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(15151);
	addr.sin6_addr = in6addr_any;

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		log(FATAL, "failed to bind TCP socket: %s", NET_ERR_STR);
		close(fd);
		exit(1);
	}

	if (listen(fd, 20) < 0) {
		log(FATAL, "failed to listen on TCP socket: %s", NET_ERR_STR);
		close(fd);
		exit(1);
	}

	std::thread event_thread(&TCPFrontend::event_thread_func, this);
	this->event_thread = std::move(event_thread);
}

TCPFrontend::~TCPFrontend() {
	event_thread_destroy = true;
	NotifyEventThread();
	event_thread.join();
	close(fd);
}

void TCPFrontend::event_thread_func() {
	fd_set readfds;
	fd_set writefds;
	int max_fd = 0;
	while (!event_thread_destroy) {
		log(DEBUG, "tcp event thread loop");
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		max_fd = std::max(max_fd, fd);
		FD_SET(fd, &readfds);

		for (auto &c : clients) {
			FD_SET(c->fd, &readfds);
			max_fd = std::max(max_fd, c->fd);
			if (c->out_buffer.size() > 0) {
				FD_SET(c->fd, &writefds);
			}
		}

		/*if (max_fd > FD_SETSIZE) {
			log(FATAL, "FD is higher than set size: %d %d", max_fd, FD_SETSIZE);
			exit(1);
		}*/
		if (select(max_fd + 1, &readfds, &writefds, NULL, NULL) < 0) {
			log(FATAL, "failed to select file descriptors: %s", NET_ERR_STR);
			exit(1);
		}

		// Check select status on server socket
		if (FD_ISSET(fd, &readfds)) {
			log(DEBUG, "incoming connection detected on %d", fd);

			struct sockaddr_in6 client_addr;
			socklen_t client_addrlen = sizeof(client_addr);
			int client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_addrlen);
			if (client_fd < 0) {
				log(WARN, "failed to accept incoming connection");
			} else {
				std::shared_ptr<Client> client = std::make_shared<Client>(this, client_fd);
				clients.push_back(client);
				twibd->AddClient(client);
			}
		}

		// pump i/o
		for (auto ci = clients.begin(); ci != clients.end(); ci++) {
			std::shared_ptr<Client> &client = *ci;
			if (FD_ISSET(client->fd, &writefds)) {
				client->PumpOutput();
			}
			if (FD_ISSET(client->fd, &readfds)) {
				log(DEBUG, "incoming data for client %x", client->client_id);
				client->PumpInput();
			}
		}

		for (auto i = clients.begin(); i != clients.end(); ) {
			if ((*i)->deletion_flag) {
				twibd->RemoveClient(*i);
				i = clients.erase(i);
				continue;
			}

			(*i)->Process();
			i++;
		}
	}
}

void TCPFrontend::NotifyEventThread() {
	// Connect to the server in order to wake it up
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		log(FATAL, "failed to create socket: %s", NET_ERR_STR);
		exit(1);
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(15151);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		log(FATAL, "failed to connect for notification: %s", NET_ERR_STR);
		exit(1);
	}
	close(fd);
}

TCPFrontend::Client::Client(TCPFrontend *frontend, int fd) : frontend(frontend), twibd(frontend->twibd), fd(fd) {
}

TCPFrontend::Client::~Client() {
	log(DEBUG, "Closing connection %x", this->client_id);
	close(fd);
}

void TCPFrontend::Client::PumpOutput() {
	log(DEBUG, "pumping out 0x%lx bytes", out_buffer.size());
	std::lock_guard<std::mutex> lock(out_buffer_mutex);
	if (out_buffer.size() > 0) {
		ssize_t r = send(fd, out_buffer.data(), out_buffer.size(), 0);
		log(DEBUG, "Managed to send 0x%lx bytes", r);
		if (r <= 0) {
			deletion_flag = true;
			return;
		}
		if (r > 0) {
			// move everything that we didn't send to the start of the buffer
			std::move(out_buffer.begin() + r, out_buffer.end(), out_buffer.begin());
			out_buffer.resize(out_buffer.size() - r);
		}
	}
}


void TCPFrontend::Client::PumpInput() {
	size_t old_size = in_buffer.size();

	// make some space to read into
	if(in_buffer.size() < in_buffer_size_hint) {
		in_buffer.resize(in_buffer_size_hint);
	} else {
		in_buffer.resize(old_size + 4096);
	}

	// read into the space we just made
	ssize_t r = recv(fd, in_buffer.data() + old_size, in_buffer.size() - old_size, 0);
	if(r < 0) {
		deletion_flag = true;
		return;
	} else { // TODO: r == 0
		// set the size of the buffer to reflect what we read
		in_buffer.resize(old_size + r);
	}
}

void TCPFrontend::Client::Process() {
	while(in_buffer.size() > 0) {
		if(in_buffer.size() < sizeof(protocol::MessageHeader)) {
			in_buffer_size_hint = sizeof(protocol::MessageHeader);
			return;
		}
		protocol::MessageHeader &mh = *(protocol::MessageHeader*) in_buffer.data();
		size_t total_message_size = sizeof(protocol::MessageHeader) + mh.payload_size;
		if(in_buffer.size() < total_message_size) {
			in_buffer_size_hint = total_message_size;
			return;
		}
		
		std::vector<uint8_t> payload(
			in_buffer.begin() + sizeof(protocol::MessageHeader),
			in_buffer.begin() + total_message_size);
		twibd->PostRequest(Request(client_id, mh.device_id, mh.object_id, mh.command_id, mh.tag, payload));
		
		// move everything past the end of the message we just processed to the start of the buffer
		std::move(in_buffer.begin() + total_message_size, in_buffer.end(), in_buffer.begin());
		in_buffer.resize(in_buffer.size() - total_message_size);
	}
}

void TCPFrontend::Client::PostResponse(Response &r) {
	std::lock_guard<std::mutex> lock(out_buffer_mutex);
	protocol::MessageHeader mh;
	mh.device_id = r.device_id;
	mh.object_id = r.object_id;
	mh.result_code = r.result_code;
	mh.tag = r.tag;
	mh.payload_size = r.payload.size();

	uint8_t *mh_bytes = (uint8_t*) &mh;
	out_buffer.insert(out_buffer.end(), mh_bytes, mh_bytes + sizeof(mh));
	out_buffer.insert(out_buffer.end(), r.payload.begin(), r.payload.end());

	frontend->NotifyEventThread();
}

} // namespace frontend
} // namespace twibd
} // namespace twili
