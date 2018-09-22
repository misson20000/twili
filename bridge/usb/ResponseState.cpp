#include "bridge/usb/USBBridge.hpp"
#include "bridge/Object.hpp"
#include "bridge/ResponseOpener.hpp"

#include "err.hpp"

namespace twili {
namespace bridge {
namespace usb {

using trn::ResultError;

USBBridge::ResponseState::ResponseState(USBBridge &bridge, uint32_t client_id, uint32_t tag) :
	detail::ResponseState(client_id, tag),
	bridge(bridge) {
	
}

size_t USBBridge::ResponseState::GetMaxTransferSize() {
	return bridge.response_data_buffer.size;
}

void USBBridge::ResponseState::SendHeader(protocol::MessageHeader &hdr) {
	memcpy(
		bridge.response_meta_buffer.data,
		&hdr,
		sizeof(hdr));
	
	PostBufferSync(
		bridge.endpoint_response_meta,
		bridge.response_meta_buffer.data,
		sizeof(hdr));
}

void USBBridge::ResponseState::SendData(uint8_t *data, size_t size) {
	auto max_size = bridge.response_data_buffer.size;
	while(size > max_size) {
		SendData(data, max_size);
		data+= max_size;
		size-= max_size;
	}
	memcpy(bridge.response_data_buffer.data, data, size);
	USBBridge::PostBufferSync(bridge.endpoint_response_data, bridge.response_data_buffer.data, size);
	transferred_size+= size;
}

void USBBridge::ResponseState::Finalize() {
	if(transferred_size != total_size) {
		throw ResultError(TWILI_ERR_BAD_RESPONSE);
	}
	if(objects.size() != object_count) {
		throw ResultError(TWILI_ERR_BAD_RESPONSE);
	}

	if(object_count > 0) {
		// send object IDs
		uint32_t *out = (uint32_t*) bridge.response_data_buffer.data;
		for(auto p : objects) {
			*(out++) = p->object_id;
		}
		USBBridge::PostBufferSync(bridge.endpoint_response_data, bridge.response_data_buffer.data, object_count * sizeof(uint32_t));
	}
}

uint32_t USBBridge::ResponseState::ReserveObjectId() {
	return bridge.object_id++;
}

void USBBridge::ResponseState::InsertObject(std::pair<uint32_t, std::shared_ptr<Object>> &&pair) {
	bridge.objects.insert(pair);
}

} // namespace usb
} // namespace bridge
} // namespace twili
