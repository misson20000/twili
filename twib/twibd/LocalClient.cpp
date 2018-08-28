#include "LocalClient.hpp"

#include "Twibd.hpp"

namespace twili {
namespace twibd {

LocalClient::LocalClient(Twibd *twibd) : twibd(twibd) {
}

std::future<Response> LocalClient::SendRequest(Request rq) {
	std::future<Response> future;
	{
		std::lock_guard<std::mutex> lock(response_map_mutex);
		static std::random_device rng;
		
		uint32_t tag = rng();
		rq.tag = tag;
		rq.client = shared_from_this();
		
		std::promise<Response> promise;
		future = promise.get_future();
		response_map[tag] = std::move(promise);
	}

	twibd->PostRequest(std::move(rq));
	
	return future;
}

void LocalClient::PostResponse(Response &r) {
	std::promise<Response> promise;
	{
		std::lock_guard<std::mutex> lock(response_map_mutex);
		auto it = response_map.find(r.tag);
		if(it == response_map.end()) {
			LogMessage(Warning, "dropping response for unknown tag 0x%x", r.tag);
			return;
		}
		promise.swap(it->second);
		response_map.erase(it);
	}
	promise.set_value(r);
}


} // namespace twibd
} // namespace twili
