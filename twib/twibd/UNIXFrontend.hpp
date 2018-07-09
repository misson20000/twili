#pragma once

#include<list>
#include<vector>
#include<thread>
#include<mutex>

#include<poll.h>
#include<stdint.h>

#include "Messages.hpp"
#include "Protocol.hpp"
#include "Buffer.hpp"

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
		util::Buffer in_buffer;

		std::mutex out_buffer_mutex;
		util::Buffer out_buffer;

		bool has_current_mh = false;
		protocol::MessageHeader current_mh;
		util::Buffer current_payload;
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
