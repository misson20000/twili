#pragma once

#ifdef _WIN32
#include<winsock2.h>
#else
#include<sys/select.h>
#endif

#include<list>
#include<vector>
#include<thread>
#include<mutex>

#include<stdint.h>

#include "Messages.hpp"

namespace twili {
namespace twibd {

class Twibd;

namespace frontend {

class TCPFrontend {
 public:
	TCPFrontend(Twibd *twibd);
	~TCPFrontend();

	class Client : public twibd::Client {
	 public:
		Client(TCPFrontend *frontend, int fd);
		~Client();
		void PumpOutput();
		void PumpInput();
		void Process();

		virtual void PostResponse(Response &r);

		TCPFrontend *frontend;
		Twibd *twibd;
		int fd;
		size_t in_buffer_size_hint = 0;
		std::vector<char> in_buffer;

		std::mutex out_buffer_mutex;
		std::vector<char> out_buffer;
	};
	
 private:
	Twibd *twibd;
	int fd;
	
	bool event_thread_destroy = false;
	void event_thread_func();
	std::thread event_thread;

	std::list<std::shared_ptr<Client>> clients;
	
	void NotifyEventThread();
};

} // namespace frontend
} // namespace twibd
} // namespace twili
