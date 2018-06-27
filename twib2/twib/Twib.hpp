#pragma once

#include<thread>
#include<future>
#include<mutex>
#include<map>

#include "Messages.hpp"

namespace twili {
namespace twib {

class Twib {
 public:
	Twib();
	~Twib();

	std::future<Response> SendRequest(Request rq);
 private:
	std::mutex twibd_mutex;
	int fd;

	bool event_thread_destroy = false;
	void event_thread_func();
	std::thread event_thread;
	int event_thread_notification_pipe[2];

	void NotifyEventThread();

	void PumpOutput();
	void PumpInput();
	void ProcessResponses();
	
	size_t in_buffer_size_hint = 0;
	std::vector<uint8_t> in_buffer;

	std::mutex out_buffer_mutex;
	std::vector<uint8_t> out_buffer;
	
	std::map<uint32_t, std::promise<Response>> response_map;
	std::mutex response_map_mutex;
};

} // namespace twib
} // namespace twili
