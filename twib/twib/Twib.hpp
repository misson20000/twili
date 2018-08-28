#pragma once

#include "platform.hpp"

#include<memory>
#include<array>
#include<thread>
#include<future>
#include<mutex>
#include<map>

#include "Buffer.hpp"
#include "Protocol.hpp"
#include "Messages.hpp"
#include "MessageConnection.hpp"

namespace twili {
namespace twib {

class ITwibMetaInterface;
class ITwibDeviceInterface;

class Twib;

class Client : public std::enable_shared_from_this<Client> {
	public:
	Client(twibc::MessageConnection<Client> &mc, Twib *twib);

	void IncomingMessage(protocol::MessageHeader &mh, util::Buffer &buffer, util::Buffer &object_ids);
	std::future<Response> SendRequest(Request rq);
	
	bool deletion_flag;
	private:
	twibc::MessageConnection<Client> &mc;
	Twib *twib;
	std::map<uint32_t, std::promise<Response>> response_map;
	std::mutex response_map_mutex;
};

class Twib {
 public:
	Twib(int tcp);
	~Twib();
	
	twibc::MessageConnection<Client> mc;
	void NotifyEventThread();
 private:
	bool event_thread_destroy = false;
	void event_thread_func();
	std::thread event_thread;
	int event_thread_notification_pipe[2];
};

} // namespace twib
} // namespace twili
