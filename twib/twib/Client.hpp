#pragma once

#include<future>
#include<mutex>
#include<map>

#include "Messages.hpp"
#include "Protocol.hpp"
#include "Buffer.hpp"

namespace twili {
namespace twib {
namespace client {

class Client {
 public:
	std::future<Response> SendRequest(Request &&rq);
	
	bool deletion_flag = false;
	
 protected:
	virtual void SendRequestImpl(const Request &rq) = 0;
	void PostResponse(protocol::MessageHeader &mh, util::Buffer &payload, util::Buffer &object_ids);
 private:
	std::map<uint32_t, std::promise<Response>> response_map;
	std::mutex response_map_mutex;
};

} // namespace client
} // namespace twib
} // namespace twili
