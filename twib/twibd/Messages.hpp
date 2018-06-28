#pragma once

#include<vector>
#include<memory>

#include<stdint.h>

namespace twili {
namespace twibd {

class Response {
 public:
	Response();
	Response(uint32_t client_id, uint32_t device_id, uint32_t object_id, uint32_t result_code, uint32_t tag, std::vector<uint8_t> payload);
	
	uint32_t client_id;
	uint32_t device_id;
	uint32_t object_id;
	uint32_t result_code;
	uint32_t tag;
	std::vector<uint8_t> payload;
};

class Client {
 public:
	uint32_t client_id;
	bool deletion_flag = false;
	virtual void PostResponse(Response &r) = 0;
};

class Request {
 public:
	Request();
	Request(uint32_t client_id, uint32_t device_id, uint32_t object_id, uint32_t command_id, uint32_t tag, std::vector<uint8_t> payload);
	Response RespondError(uint32_t code);
	Response RespondOk();
	
	uint32_t client_id;
	uint32_t device_id;
	uint32_t object_id;
	uint32_t command_id;
	uint32_t tag;
	std::vector<uint8_t> payload;
 private:
};

} // namespace twibd
} // namespace twili
