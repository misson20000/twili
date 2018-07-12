#pragma once

#include<list>
#include<vector>
#include<thread>
#include<mutex>

#ifdef _WIN32
#include<winsock2.h>
#else
#include<sys/socket.h>
#include<sys/select.h>
#endif

#include<stdint.h>

#include "Messages.hpp"
#include "Protocol.hpp"
#include "Buffer.hpp"

#ifndef _WIN32
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket close
#endif

namespace twili {
namespace twibd {

class Twibd;

namespace frontend {

class SocketFrontend {
	public:
	SocketFrontend(Twibd *twibd, int address_family, int socktype, struct sockaddr *bind_addr, size_t bind_addrlen);
	~SocketFrontend();

	class Client : public twibd::Client {
		public:
		Client(SocketFrontend *frontend, SOCKET fd);
		~Client();
		void PumpOutput();
		void PumpInput();
		void Process();

		virtual void PostResponse(Response &r);

		SocketFrontend *frontend;
		Twibd *twibd;
		SOCKET fd;
		util::Buffer in_buffer;

		std::mutex out_buffer_mutex;
		util::Buffer out_buffer;

		bool has_current_mh = false;
		protocol::MessageHeader current_mh;
		util::Buffer current_payload;
	};

	private:
	Twibd *twibd;
	SOCKET fd;

	int address_family;
	int socktype;
	struct sockaddr_storage bind_addr;
	size_t bind_addrlen;
	void UnlinkIfUnix();
	
	bool event_thread_destroy = false;
	void event_thread_func();
	std::thread event_thread;
#ifndef _WIN32
	int event_thread_notification_pipe[2];
#endif
	
	std::list<std::shared_ptr<Client>> clients;

	void NotifyEventThread();
};

} // namespace frontend
} // namespace twibd
} // namespace twili
