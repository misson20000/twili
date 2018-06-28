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

	msgpack11::MsgPack identification;
	std::string device_nickname;
	std::string serial_number;
	bool deletion_flag = false;
	uint32_t device_id;
};

} // namespace twibd
} // namespace twili
