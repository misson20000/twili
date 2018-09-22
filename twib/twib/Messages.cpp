#include "Messages.hpp"

namespace twili {
namespace twib {

Response::Response(std::shared_ptr<Client> client, uint32_t device_id, uint32_t object_id, uint32_t result_code, uint32_t tag, std::vector<uint8_t> payload, std::vector<std::shared_ptr<RemoteObject>> objects) :
	client(client), device_id(device_id), object_id(object_id),
	result_code(result_code), tag(tag), payload(payload),
	objects(objects) {
}

Request::Request(uint32_t device_id, uint32_t object_id, uint32_t command_id, uint32_t tag, std::vector<uint8_t> payload) :
	device_id(device_id), object_id(object_id),
	command_id(command_id), tag(tag), payload(payload) {
}

Request::Request() {
	
}

} // namespace twib
} // namespace twili
