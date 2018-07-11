#pragma once

#include <array>
#include<thread>
#include<future>
#include<mutex>
#include<map>

#include "Buffer.hpp"
#include "Protocol.hpp"
#include "Messages.hpp"

namespace twili {
namespace twib {

class ITwibMetaInterface;
class ITwibDeviceInterface;

class Twib {
 public:
	Twib(int tcp);
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
	
	util::Buffer in_buffer;
	
	std::mutex out_buffer_mutex;
	util::Buffer out_buffer;

	bool has_current_mh = false;
	protocol::MessageHeader current_mh;
	util::Buffer current_payload;
	
	std::map<uint32_t, std::promise<Response>> response_map;
	std::mutex response_map_mutex;
};

} // namespace twib
} // namespace twili
