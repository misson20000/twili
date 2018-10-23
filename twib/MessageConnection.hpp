#pragma once

#include "platform.hpp"

#include<mutex>
#include<memory>
#include<optional>

#include "Protocol.hpp"
#include "Buffer.hpp"
#include "Logger.hpp"

namespace twili {
namespace twibc {

class MessageConnection {
 public:
	MessageConnection();
	virtual ~MessageConnection();

	class Request {
	 public:
		protocol::MessageHeader mh;
		util::Buffer payload;
		util::Buffer object_ids;
	};

	// The use of a pointer here is truly lamentable. I would've much preferred to use std::optional<Request&>
	Request *Process(); // NULL pointer means no message
	void SendMessage(const protocol::MessageHeader &mh, const std::vector<uint8_t> &payload, const std::vector<uint32_t> &object_ids);
	
 protected:
	util::Buffer in_buffer;

	std::mutex out_buffer_mutex;
	util::Buffer out_buffer;

	virtual void SignalInput() = 0;
	virtual void SignalOutput() = 0;

 private:
	Request current_rq;
	bool has_current_mh = false;
	bool has_current_payload = false;
};

}
}
