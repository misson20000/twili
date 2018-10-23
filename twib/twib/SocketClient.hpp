#pragma once

#include "Client.hpp"

#include "Buffer.hpp"
#include "SocketMessageConnection.hpp"

namespace twili {
namespace twib {
namespace client {

class SocketClient : public Client {
 public:
	SocketClient(SOCKET fd);
	~SocketClient();
	
 protected:
	virtual void SendRequestImpl(const Request &rq) override;
 private:
	void NotifyEventThread();
	bool event_thread_destroy = false;
	void event_thread_func();
	std::thread event_thread;
	int event_thread_notification_pipe[2];

	class EventThreadNotifier : public twibc::SocketMessageConnection::EventThreadNotifier {
	 public:
		EventThreadNotifier(SocketClient &client);
		void Notify();
	 private:
		SocketClient &client;
	} notifier;

	twibc::SocketMessageConnection connection;
};

} // namespace client
} // namespace twib
} // namespace twili
