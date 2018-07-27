#pragma once

#include<memory>

#include "BridgeObject.hpp"
#include "TwibPipe.hpp"

namespace twili {
namespace bridge {

class ITwibPipeReader : public bridge::Object {
 public:
	ITwibPipeReader(uint32_t object_id, std::weak_ptr<TwibPipe> pipe);
	virtual void HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener);
 private:
	std::weak_ptr<TwibPipe> pipe;

	void Read(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener);
};

} // namespace bridge
} // namespace twili
