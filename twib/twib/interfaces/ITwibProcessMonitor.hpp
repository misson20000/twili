#pragma once

#include<vector>

#include "../RemoteObject.hpp"
#include "ITwibPipeWriter.hpp"
#include "ITwibPipeReader.hpp"

namespace twili {
namespace twib {

class ITwibProcessMonitor {
 public:
	ITwibProcessMonitor(std::shared_ptr<RemoteObject> obj);

	uint64_t Launch();
	void AppendCode(std::vector<uint8_t> code);
	ITwibPipeWriter OpenStdin();
	ITwibPipeReader OpenStdout();
	ITwibPipeReader OpenStderr();
	uint32_t WaitStateChange();
 private:
	std::shared_ptr<RemoteObject> obj;
};

} // namespace twib
} // namespace twili
