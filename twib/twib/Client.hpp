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
	virtual ~Client() = default;
	std::future<Response> SendRequest(Request &&rq);
	
	bool deletion_flag = false;
	
 protected:
	virtual void SendRequestImpl(const Request &rq) = 0;
	void PostResponse(protocol::MessageHeader &mh, util::Buffer &payload, util::Buffer &object_ids);
	void FailAllRequests(uint32_t code);
 private:
	std::map<uint32_t, std::promise<Response>> response_map;
	std::mutex response_map_mutex;
};

} // namespace client
} // namespace twib
} // namespace twili
