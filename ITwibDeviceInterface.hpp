#pragma once

#include "bridge/Object.hpp"
#include "bridge/ResponseOpener.hpp"

namespace twili {

class Twili;

namespace bridge {

class ITwibDeviceInterface : public bridge::Object {
 public:
	ITwibDeviceInterface(uint32_t object_id, Twili &twili);
	virtual void HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, bridge::ResponseOpener opener);
 private:
	Twili &twili;
	
	void Run(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void Reboot(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void CoreDump(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void Terminate(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void ListProcesses(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void UpgradeTwili(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void Identify(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void ListNamedPipes(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void OpenNamedPipe(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void OpenActiveDebugger(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
};

} // namespace bridge
} // namespace twili
