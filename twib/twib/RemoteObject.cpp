#include "RemoteObject.hpp"

#include "Logger.hpp"

namespace twili {
namespace twib {

RemoteObject::RemoteObject(Twib *twib, uint32_t device_id, uint32_t object_id) :
	twib(twib), device_id(device_id), object_id(object_id) {
	
}

RemoteObject::~RemoteObject() {
}

std::future<Response> RemoteObject::SendRequest(uint32_t command_id, std::vector<uint8_t> payload) {
	return twib->SendRequest(Request(device_id, object_id, command_id, 0, payload));
}

Response RemoteObject::SendSyncRequest(uint32_t command_id, std::vector<uint8_t> payload) {
	Response rs = SendRequest(command_id, payload).get();
	if(rs.result_code != 0) {
		LogMessage(Fatal, "got result code 0x%x from device", rs.result_code);
		exit(1);
	}
	return rs;
}

RemoteObject RemoteObject::CreateSiblingFromId(uint32_t object_id) {
	return RemoteObject(twib, device_id, object_id);
}

} // namespace twib
} // namespace twili
