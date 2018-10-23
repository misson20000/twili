#pragma once

#include "platform.hpp"

#include<list>
#include<vector>
#include<thread>
#include<mutex>

#include<stdint.h>

#include "EventThreadNotifier.hpp"

namespace twili {
namespace twibc {

class SocketServer {
 public:
	class Logic {
	 public:
		virtual void Prepare(SocketServer &server) = 0;
	};
	
	SocketServer(Logic &logic);
	~SocketServer();

	class Socket {
	 public:
		Socket();
		Socket(SOCKET fd);
		~Socket();

		Socket &operator=(SOCKET fd);

		bool IsValid();
		void Close();
		
		virtual bool WantsRead();
		virtual bool WantsWrite();
		virtual void SignalRead();
		virtual void SignalWrite();
		virtual void SignalError();

		SOCKET fd;
	 private:
		bool is_valid = false;
	};

	void Begin();
	void Destroy();
	void Clear();
	void AddSocket(Socket &socket);

	class EventThreadNotifier : public twibc::EventThreadNotifier {
	 public:
		EventThreadNotifier(SocketServer &server);
		virtual void Notify() const override;
	 private:
		SocketServer &server;
	} const notifier;
	
 private:
	std::vector<std::reference_wrapper<Socket>> sockets;
	Logic &logic;
	
	bool event_thread_destroy = false;
	void event_thread_func();
	std::thread event_thread;
#ifndef _WIN32
	int event_thread_notification_pipe[2];
#endif
};

} // namespace twibc
} // namespace twili
