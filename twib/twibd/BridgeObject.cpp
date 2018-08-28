#include "BridgeObject.hpp"

#include "Twibd.hpp"
#include "Messages.hpp"

namespace twili {
namespace twibd {

BridgeObject::BridgeObject(Twibd &twibd, uint32_t device_id, uint32_t object_id) :
	twibd(twibd),
	device_id(device_id),
	object_id(object_id) {
}

BridgeObject::~BridgeObject() {
	// try to close object if valid
	if(valid) {
		LogMessage(Debug, "cleaning up lost object 0x%x", object_id);
		twibd.local_client->SendRequest(
			Request(
				nullptr, // local client overwrites this
				device_id,
				object_id,
				0xffffffff,
				0 // local client generates this
				)); // we don't care about the response
	}
}

} // namespace twibd
} // namespace twili
