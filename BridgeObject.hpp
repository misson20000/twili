#pragma once

#include "USBBridge.hpp"

namespace twili {
namespace bridge {

class Object {
 public:
	Object(uint32_t object_id);
	virtual ~Object() = default;
	virtual void HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, std::shared_ptr<usb::USBBridge::ResponseOpener> opener) = 0;
	
	const uint32_t object_id;
};

} // namespace bridge
} // namespace twili
