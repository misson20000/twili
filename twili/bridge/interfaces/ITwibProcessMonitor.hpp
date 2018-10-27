#pragma once

#include "../Object.hpp"
#include "../ResponseOpener.hpp"
#include "../../process/MonitoredProcess.hpp"

namespace twili {

class Twili;

namespace bridge {

class ITwibProcessMonitor : public bridge::Object {
 public:
	ITwibProcessMonitor(uint32_t object_id, std::shared_ptr<process::MonitoredProcess> process);
	virtual ~ITwibProcessMonitor() override;
	
	virtual void HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, bridge::ResponseOpener opener) override;
 private:
	void Launch(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void Terminate(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void AppendCode(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	
	void OpenStdin(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void OpenStdout(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void OpenStderr(std::vector<uint8_t> payload, bridge::ResponseOpener opener);

	std::shared_ptr<process::MonitoredProcess> process;
};

} // namespace bridge
} // namespace twili
