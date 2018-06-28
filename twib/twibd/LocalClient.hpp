#pragma once

#include<future>
#include<map>
#include<mutex>

#include "Messages.hpp"

namespace twili {
namespace twibd {

class Twibd;

class LocalClient : public twibd::Client {
 public:
	LocalClient(Twibd *twibd);

	std::future<Response> SendRequest(Request rq);
	virtual void PostResponse(Response &r);

	Twibd *twibd;

	std::map<uint32_t, std::promise<Response>> response_map;
	std::mutex response_map_mutex;
};

} // namespace twibd
} // namespace twili
