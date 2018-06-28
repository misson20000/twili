#include "Messages.hpp"

namespace twili {
namespace twibd {

Response::Response() {
}

Response::Response(uint32_t client_id, uint32_t device_id, uint32_t object_id, uint32_t result_code, uint32_t tag, std::vector<uint8_t> payload) :
	client_id(client_id), device_id(device_id), object_id(object_id),
	result_code(result_code), tag(tag), payload(payload) {
}

Request::Request() {
}

Request::Request(uint32_t client_id, uint32_t device_id, uint32_t object_id, uint32_t command_id, uint32_t tag, std::vector<uint8_t> payload) :
	client_id(client_id), device_id(device_id), object_id(object_id),
	command_id(command_id), tag(tag), payload(payload) {
}

Response Request::RespondError(uint32_t code) {
	return Response(client_id, device_id, object_id, code, tag, std::vector<uint8_t>());
}

Response Request::RespondOk() {
	return RespondError(0);
}

} // namespace twibd
} // namespace twili
