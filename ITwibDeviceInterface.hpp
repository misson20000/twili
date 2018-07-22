#pragma once

#include "BridgeObject.hpp"

namespace twili {
namespace bridge {

class ITwibDeviceInterface : public bridge::Object {
 public:
	ITwibDeviceInterface(uint32_t object_id, Twili &twili);
	virtual void HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, std::shared_ptr<usb::USBBridge::ResponseOpener> opener);
 private:
	Twili &twili;
	
	trn::Result<std::nullopt_t> Run(std::vector<uint8_t> payload, std::shared_ptr<usb::USBBridge::ResponseOpener> opener);
	trn::Result<std::nullopt_t> Reboot(std::vector<uint8_t> payload, std::shared_ptr<usb::USBBridge::ResponseOpener> opener);
	trn::Result<std::nullopt_t> CoreDump(std::vector<uint8_t> payload, std::shared_ptr<usb::USBBridge::ResponseOpener> opener);
	trn::Result<std::nullopt_t> Terminate(std::vector<uint8_t> payload, std::shared_ptr<usb::USBBridge::ResponseOpener> opener);
	trn::Result<std::nullopt_t> ListProcesses(std::vector<uint8_t> payload, std::shared_ptr<usb::USBBridge::ResponseOpener> opener);
	trn::Result<std::nullopt_t> UpgradeTwili(std::vector<uint8_t> payload, std::shared_ptr<usb::USBBridge::ResponseOpener> opener);
	trn::Result<std::nullopt_t> Identify(std::vector<uint8_t> payload, std::shared_ptr<usb::USBBridge::ResponseOpener> opener);
};

} // namespace bridge
} // namespace twili
