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

#include<string>
#include<stdint.h>
#include<msgpack11.hpp>

#include "Messages.hpp"

namespace twili {
namespace twibd {

class Device {
 public:
	virtual void SendRequest(const Request &&r) = 0;
	virtual int GetPriority() = 0;
	virtual std::string GetBridgeType() = 0;
	
	msgpack11::MsgPack identification;
	std::string device_nickname;
	std::string serial_number;
	bool deletion_flag = false;
	uint32_t device_id;
};

} // namespace twibd
} // namespace twili
