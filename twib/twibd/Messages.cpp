#include "Messages.hpp"

namespace twili {
namespace twibd {

Response::Response() {
}

Response::Response(uint32_t client_id, uint32_t device_id, uint32_t object_id, uint32_t result_code, uint32_t tag, std::vector<uint8_t> payload) :
	client_id(client_id), device_id(device_id), object_id(object_id),
	result_code(result_code), tag(tag), payload(payload) {
}

Response::Response(uint32_t client_id, uint32_t device_id, uint32_t object_id, uint32_t result_code, uint32_t tag) :
	client_id(client_id), device_id(device_id), object_id(object_id),
	result_code(result_code), tag(tag) {
}

WeakRequest::WeakRequest() {
}

WeakRequest::WeakRequest(uint32_t client_id, uint32_t device_id, uint32_t object_id, uint32_t command_id, uint32_t tag, std::vector<uint8_t> payload) :
	client_id(client_id), device_id(device_id), object_id(object_id),
	command_id(command_id), tag(tag), payload(payload) {
}

WeakRequest::WeakRequest(uint32_t client_id, uint32_t device_id, uint32_t object_id, uint32_t command_id, uint32_t tag) :
	client_id(client_id), device_id(device_id), object_id(object_id),
	command_id(command_id), tag(tag) {
}

Response WeakRequest::RespondError(uint32_t code) {
	return Response(client_id, device_id, object_id, code, tag, std::vector<uint8_t>());
}

Response WeakRequest::RespondOk() {
	return RespondError(0);
}

Request::Request() {
}

Request::Request(std::shared_ptr<Client> client, uint32_t device_id, uint32_t object_id, uint32_t command_id, uint32_t tag, std::vector<uint8_t> payload) :
	client(client), device_id(device_id), object_id(object_id),
	command_id(command_id), tag(tag), payload(payload) {
}

Request::Request(std::shared_ptr<Client> client, uint32_t device_id, uint32_t object_id, uint32_t command_id, uint32_t tag) :
	client(client), device_id(device_id), object_id(object_id),
	command_id(command_id), tag(tag) {
}

Response Request::RespondError(uint32_t code) {
	return Response(client->client_id, device_id, object_id, code, tag, std::vector<uint8_t>());
}

Response Request::RespondOk() {
	return RespondError(0);
}

WeakRequest Request::Weak() const {
	return WeakRequest(client ? client->client_id : 0xffffffff, device_id, object_id, command_id, tag, payload);
}

} // namespace twibd
} // namespace twili
