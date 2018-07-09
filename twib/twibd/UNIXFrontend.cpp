#include "UNIXFrontend.hpp"

#include<algorithm>

#include<sys/socket.h>
#include<sys/un.h>
#include<string.h>
#include<unistd.h>

#include "Twibd.hpp"
#include "Protocol.hpp"
#include "config.hpp"

namespace twili {
namespace twibd {
namespace frontend {

UNIXFrontend::UNIXFrontend(Twibd *twibd) : twibd(twibd) {
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(fd < 0) {
		log(FATAL, "failed to create UNIX domain socket: %s", strerror(errno));
		exit(1);
	}
	log(DEBUG, "created UNIX domain socket: %d", fd);

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, Twibd_UNIX_SOCKET_PATH, sizeof(addr.sun_path)-1);

	unlink(Twibd_UNIX_SOCKET_PATH);
	if(bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		log(FATAL, "failed to bind UNIX domain socket: %s", strerror(errno));
		close(fd);
		exit(1);
	}

	if(listen(fd, 20) < 0) {
		log(FATAL, "failed to listen on UNIX domain socket: %s", strerror(errno));
		close(fd);
		unlink(Twibd_UNIX_SOCKET_PATH);
		exit(1);
	}
	
	if(pipe(event_thread_notification_pipe) < 0) {
		log(FATAL, "failed to create pipe for event thread notifications: %s", strerror(errno));
		close(fd);
		unlink(Twibd_UNIX_SOCKET_PATH);
		exit(1);
	}
	
	std::thread event_thread(&UNIXFrontend::event_thread_func, this);
	this->event_thread = std::move(event_thread);
}

UNIXFrontend::~UNIXFrontend() {
	event_thread_destroy = true;
	NotifyEventThread();
	event_thread.join();

	close(fd);
	unlink(Twibd_UNIX_SOCKET_PATH);
}

void UNIXFrontend::event_thread_func() {
	while(!event_thread_destroy) {
		log(DEBUG, "unix event thread loop");

		pollfds.clear();
		pollfds.push_back({.fd = event_thread_notification_pipe[0], .events = POLLIN});
		pollfds.push_back({.fd = fd, .events = POLLIN});

		for(auto &c : clients) {
			short int events = POLLIN;
			if(c->out_buffer.ReadAvailable() > 0) {
				events|= POLLOUT;
			}
			pollfds.push_back({.fd = c->fd, .events = events});
		}
		
		if(poll(pollfds.data(), pollfds.size(), -1) < 0) {
			log(FATAL, "failed to poll file descriptors: %s", strerror(errno));
			exit(1);
		}

		// check poll flags on event notification pipe
		if(pollfds[0].revents & POLLIN) {
			char buf[64];
			ssize_t r = read(event_thread_notification_pipe[0], buf, sizeof(buf));
			if(r < 0) {
				log(FATAL, "failed to read from event thread notification pipe: %s", strerror(errno));
				exit(1);
			}
			log(DEBUG, "event thread notified: '%.*s'", r, buf);
		}

		if(pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			log(FATAL, "event thread notification pipe has bad revents flags: 0x%x", pollfds[0].revents);
			exit(1);
		}

		// check poll flags on server socket
		if(pollfds[1].revents & POLLIN) {
			log(DEBUG, "incoming connection detected");

			struct sockaddr_un client_addr;
			socklen_t client_addrlen = sizeof(client_addr);
			int client_fd = accept(fd, (struct sockaddr *) &client_addr, &client_addrlen);
			if(client_fd < 0) {
				log(WARN, "failed to accept incoming connection");
			}
			std::shared_ptr<Client> client = std::make_shared<Client>(this, client_fd);
			clients.push_back(client);
			twibd->AddClient(client);
		}

		if(pollfds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			log(FATAL, "server socket has bad revents flags: 0x%x", pollfds[1].revents);
			exit(1);
		}

		// pump i/o
		int ni = 2;
		for(auto ci = clients.begin(); ni < pollfds.size(); ni++, ci++) {
			std::shared_ptr<Client> &client = *ci;
			if(pollfds[ni].revents & POLLOUT) {
				client->PumpOutput();
			}
			if(pollfds[ni].revents & POLLIN) {
				log(DEBUG, "incoming data for client %d", ni-2);
				client->PumpInput();
			}
			if(pollfds[ni].revents & (POLLERR | POLLHUP | POLLNVAL)) {
				log(INFO, "client %d poll errored", ni-2);
				client->deletion_flag = true;
			}
		}

		for(auto i = clients.begin(); i != clients.end(); ) {
			if((*i)->deletion_flag) {
				twibd->RemoveClient(*i);
				i = clients.erase(i);
				continue;
			}
			
			(*i)->Process();
			
			i++;
		}
	}
}

void UNIXFrontend::NotifyEventThread() {
	char buf[] = ".";
	if(write(event_thread_notification_pipe[1], buf, sizeof(buf)) != sizeof(buf)) {
		log(FATAL, "failed to write to event thread notification pipe: %s", strerror(errno));
		exit(1);
	}
}

UNIXFrontend::Client::Client(UNIXFrontend *frontend, int fd) : frontend(frontend), twibd(frontend->twibd), fd(fd) {
}

UNIXFrontend::Client::~Client() {
	close(fd);
}

void UNIXFrontend::Client::PumpOutput() {
	log(DEBUG, "pumping out 0x%lx bytes", out_buffer.ReadAvailable());
	std::lock_guard<std::mutex> lock(out_buffer_mutex);
	if(out_buffer.ReadAvailable() > 0) {
		ssize_t r = send(fd, out_buffer.Read(), out_buffer.ReadAvailable(), 0);
		if(r < 0) {
			deletion_flag = true;
			return;
		}
		if(r > 0) {
			out_buffer.MarkRead(r);
		}
	}
}

void UNIXFrontend::Client::PumpInput() {
	std::tuple<uint8_t*, size_t> target = in_buffer.Reserve(8192);
	ssize_t r = recv(fd, std::get<0>(target), std::get<1>(target), 0);
	if(r < 0) {
		deletion_flag = true;
		return;
	} else {
		in_buffer.MarkWritten(r);
	}
}

void UNIXFrontend::Client::Process() {
	while(in_buffer.ReadAvailable() > 0) {
		if(!has_current_mh) {
			if(in_buffer.Read(current_mh)) {
				has_current_mh = true;
			} else {
				in_buffer.Reserve(sizeof(protocol::MessageHeader));
				return;
			}
		}

		current_payload.Clear();
		if(in_buffer.Read(current_payload, current_mh.payload_size)) {
			twibd->PostRequest(
				Request(
					client_id,
					current_mh.device_id,
					current_mh.object_id,
					current_mh.command_id,
					current_mh.tag,
					std::vector(current_payload.Read(), current_payload.Read() + current_payload.ReadAvailable())));
			has_current_mh = false;
		} else {
			in_buffer.Reserve(current_mh.payload_size);
			return;
		}
	}
}

void UNIXFrontend::Client::PostResponse(Response &r) {
	std::lock_guard<std::mutex> lock(out_buffer_mutex);
	protocol::MessageHeader mh;
	mh.device_id = r.device_id;
	mh.object_id = r.object_id;
	mh.result_code = r.result_code;
	mh.tag = r.tag;
	mh.payload_size = r.payload.size();

	out_buffer.Write(mh);
	out_buffer.Write(r.payload);

	frontend->NotifyEventThread();
}

} // namespace frontend
} // namespace twibd
} // namespace twili
