#pragma once

#include "platform.hpp"

#include<thread>
#include<list>
#include<queue>
#include<mutex>
#include<condition_variable>

#include "Buffer.hpp"
#include "Device.hpp"
#include "Messages.hpp"
#include "Protocol.hpp"
#include "MessageConnection.hpp"

namespace twili {
namespace twibd {

class Twibd;

namespace backend {

class TCPBackend {
 public:
	TCPBackend(Twibd *twibd);
	~TCPBackend();

	std::string Connect(std::string hostname, std::string port);
	void Connect(sockaddr *sockaddr, socklen_t addr_len);
	
	class Device : public twibd::Device, public std::enable_shared_from_this<Device> {
	 public:
		Device(twibc::MessageConnection<Device> &mc, TCPBackend *backend);
		~Device();

		void Begin();
		void IncomingMessage(protocol::MessageHeader &mh, util::Buffer &payload, util::Buffer &object_ids);
		void Identified(Response &r);
		virtual void SendRequest(const Request &&r);

		TCPBackend *backend;
		twibc::MessageConnection<Device> &connection;
		std::list<WeakRequest> pending_requests;
		Response response_in;
		bool ready_flag = false;
		bool added_flag = false;
	};

 private:
	Twibd *twibd;
	std::list<std::shared_ptr<twibc::MessageConnection<Device>>> connections;
	
	bool event_thread_destroy = false;
	void event_thread_func();
	std::thread event_thread;
};

} // namespace backend
} // namespace twibd
} // namespace twili
