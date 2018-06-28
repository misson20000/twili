#pragma once

#include<list>
#include<vector>
#include<thread>
#include<mutex>

#include<poll.h>
#include<stdint.h>

#include "Messages.hpp"

namespace twili {
namespace twibd {

class Twibd;

namespace frontend {

class UNIXFrontend {
 public:
	UNIXFrontend(Twibd *twibd);
	~UNIXFrontend();

	class Client : public twibd::Client {
	 public:
		Client(UNIXFrontend *frontend, int fd);
		~Client();
		void PumpOutput();
		void PumpInput();
		void Process();

		virtual void PostResponse(Response &r);

		UNIXFrontend *frontend;
		Twibd *twibd;
		int fd;
		size_t in_buffer_size_hint = 0;
		std::vector<uint8_t> in_buffer;

		std::mutex out_buffer_mutex;
		std::vector<uint8_t> out_buffer;
	};
	
 private:
	Twibd *twibd;
	int fd;
	
	bool event_thread_destroy = false;
	void event_thread_func();
	std::thread event_thread;
	std::vector<struct pollfd> pollfds; // to be used only by the event_thread
	int event_thread_notification_pipe[2];

	std::list<std::shared_ptr<Client>> clients;
	
	void NotifyEventThread();
};

} // namespace frontend
} // namespace twibd
} // namespace twili
