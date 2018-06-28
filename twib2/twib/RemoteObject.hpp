#pragma once

#include "Twib.hpp"

namespace twili {
namespace twib {

class RemoteObject {
 public:
	RemoteObject(Twib *twib, uint32_t device_id, uint32_t object_id);
	~RemoteObject();

	std::future<Response> SendRequest(uint32_t command_id, std::vector<uint8_t> payload);
	RemoteObject CreateSiblingFromId(uint32_t object_id);
 private:
	Twib *twib;
	uint32_t device_id;
	uint32_t object_id;
};

} // namespace twib
} // namespace twili
