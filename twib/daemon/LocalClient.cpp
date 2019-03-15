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

#include "LocalClient.hpp"

#include "Daemon.hpp"

namespace twili {
namespace twib {
namespace daemon {

LocalClient::LocalClient(Daemon &daemon) : daemon(daemon) {
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

	daemon.PostRequest(std::move(rq));
	
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

} // namespace daemon
} // namespace twib
} // namespace twili
