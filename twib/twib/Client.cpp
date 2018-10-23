#include "Client.hpp"

#include<random>

#include "Twib.hpp"
#include "RemoteObject.hpp"

namespace twili {
namespace twib {
namespace client {

void Client::PostResponse(protocol::MessageHeader &mh, util::Buffer &payload, util::Buffer &object_ids) {
	// create RAII objects for remote objects
	std::vector<std::shared_ptr<RemoteObject>> objects(mh.object_count);
	for(uint32_t i = 0; i < mh.object_count; i++) {
		uint32_t id;
		if(!object_ids.Read(id)) {
			LogMessage(Error, "not enough object IDs");
			return;
		}
		objects[i] = std::make_shared<RemoteObject>(*this, mh.device_id, id);
	}
	
	std::promise<Response> promise;
	{
		std::lock_guard<std::mutex> lock(response_map_mutex);
		auto it = response_map.find(mh.tag);
		if(it == response_map.end()) {
			LogMessage(Warning, "dropping response for unknown tag 0x%x", mh.tag);
			return;
		}
		promise.swap(it->second);
		response_map.erase(it);
	}
	
	promise.set_value(
		Response(
			mh.device_id,
			mh.object_id,
			mh.result_code,
			mh.tag,
			std::vector<uint8_t>(payload.Read(), payload.Read() + payload.ReadAvailable()),
			objects));
}

std::future<Response> Client::SendRequest(Request &&rq) {
	std::future<Response> future;
	{
		std::lock_guard<std::mutex> lock(response_map_mutex);
		static std::random_device rng;
		
		uint32_t tag = rng();
		rq.tag = tag;
		
		std::promise<Response> promise;
		future = promise.get_future();
		response_map[tag] = std::move(promise);
	}
	
	SendRequestImpl(rq);
	return future;
}

} // namespace client
} // namespace twib
} // namespace twili
