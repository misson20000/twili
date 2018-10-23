#pragma once

#include<vector>
#include<memory>

#include<stdint.h>

namespace twili {
namespace twib {
namespace client {
class Client;
}

class RemoteObject;

class Response {
 public:
	Response(uint32_t device_id, uint32_t object_id, uint32_t result_code, uint32_t tag, std::vector<uint8_t> payload, std::vector<std::shared_ptr<RemoteObject>> objects);
	Response() = default; // MSVC++ futures require this to be default constructible

	uint32_t device_id;
	uint32_t object_id;
	uint32_t result_code;
	uint32_t tag;
	std::vector<uint8_t> payload;
	std::vector<std::shared_ptr<RemoteObject>> objects;
};

class Request {
 public:
	Request();
	Request(uint32_t device_id, uint32_t object_id, uint32_t command_id, uint32_t tag, std::vector<uint8_t> payload);

	uint32_t device_id;
	uint32_t object_id;
	uint32_t command_id;
	uint32_t tag;
	std::vector<uint8_t> payload;
 private:
};

} // namespace twib
} // namespace twili
