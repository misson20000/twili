#pragma once

#include<stdint.h>

namespace twili {
namespace twibd {

class Twibd;

class BridgeObject {
 public:
	BridgeObject(Twibd &twibd, uint32_t device_id, uint32_t object_id);
	~BridgeObject();

	const Twibd &twibd;
	const uint32_t device_id;
	const uint32_t object_id;
	bool valid = true;
};

} // namespace twibd
} // namespace twili
