//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2018 misson20000 <xenotoad@xenotoad.net>
//
// This file is part of Twili.
//
// Twili is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Twili is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Twili.  If not, see <http://www.gnu.org/licenses/>.
//

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

void Client::FailAllRequests(uint32_t code) {
	for(auto i = response_map.begin(); i != response_map.end();) {
		i->second.set_value(
			Response(
				0, 0, code, 0,
				std::vector<uint8_t>(),
				std::vector<std::shared_ptr<RemoteObject>>()));
		i = response_map.erase(i);
	}
}

} // namespace client
} // namespace twib
} // namespace twili
