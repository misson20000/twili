#include "SocketClient.hpp"

namespace twili {
namespace twib {
namespace client {

SocketClient::SocketClient(SOCKET fd) : notifier(*this), connection(fd, notifier) {
	// TODO: Figure out how to fix this.
#ifndef _WIN32
	if(pipe(event_thread_notification_pipe) < 0) {
		LogMessage(Fatal, "failed to create pipe for event thread notifications: %s", NetErrStr());
		exit(1);
	}
#endif
	std::thread event_thread(&SocketClient::event_thread_func, this);
	this->event_thread = std::move(event_thread);
}

SocketClient::~SocketClient() {
	event_thread_destroy = true;
	NotifyEventThread();
	event_thread.join();
}

void SocketClient::SendRequestImpl(const Request &rq) {
	protocol::MessageHeader mh;
	mh.device_id = rq.device_id;
	mh.object_id = rq.object_id;
	mh.command_id = rq.command_id;
	mh.tag = rq.tag;
	mh.payload_size = rq.payload.size();
	mh.object_count = 0;

	connection.SendMessage(mh, rq.payload, std::vector<uint32_t>());
	LogMessage(Debug, "sent request");
}

SocketClient::EventThreadNotifier::EventThreadNotifier(SocketClient &client) : client(client) {
}

void SocketClient::EventThreadNotifier::Notify() {
	client.NotifyEventThread();
}

void SocketClient::event_thread_func() {
	fd_set recvset;
	fd_set sendset;
	SOCKET maxfd = 0;
	while(!event_thread_destroy) {
		LogMessage(Debug, "event thread loop");

		FD_ZERO(&recvset);
		FD_ZERO(&sendset);
#ifndef _WIN32
		FD_SET(event_thread_notification_pipe[0], &recvset);
		maxfd = std::max(maxfd, event_thread_notification_pipe[0]);
#endif
		FD_SET(connection.fd, &recvset);
		maxfd = std::max(maxfd, connection.fd);
		if(connection.HasOutData()) {
			FD_SET(connection.fd, &sendset);
		}

		if(select(maxfd + 1, &recvset, &sendset, NULL, NULL) < 0) {
			LogMessage(Fatal, "failed to select file descriptors: %s", NetErrStr());
			exit(1);
		}

#ifndef _WIN32
		// check poll flags on event notification pipe
		if(FD_ISSET(event_thread_notification_pipe[0], &recvset)) {
			char buf[64];
			ssize_t r = read(event_thread_notification_pipe[0], buf, sizeof(buf));
			if(r < 0) {
				LogMessage(Fatal, "failed to read from event thread notification pipe: %s", NetErrStr());
				exit(1);
			}
			LogMessage(Debug, "event thread notified: '%.*s'", r, buf);
		}
#endif

		// check poll flags on twibd socket
		if(FD_ISSET(connection.fd, &sendset)) {
			if(!connection.PumpOutput()) {
				LogMessage(Fatal, "I/O error");
				exit(1);
			}
		}
		if(FD_ISSET(connection.fd, &recvset)) {
			if(!connection.PumpInput()) {
				LogMessage(Fatal, "I/O error");
				exit(1);
			}
		}
		twibc::SocketMessageConnection::Request *rq;
		while((rq = connection.Process()) != nullptr) {
			PostResponse(rq->mh, rq->payload, rq->object_ids);
		}
	}
}

void SocketClient::NotifyEventThread() {
	// TODO: fixme for windows
	char buf[] = ".";
	if(write(event_thread_notification_pipe[1], buf, sizeof(buf)) != sizeof(buf)) {
		LogMessage(Fatal, "failed to write to event thread notification pipe: %s", NetErrStr());
		exit(1);
	}
}

} // namespace client
} // namespace twib
} // namespace twili
