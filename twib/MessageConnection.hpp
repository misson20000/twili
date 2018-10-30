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
