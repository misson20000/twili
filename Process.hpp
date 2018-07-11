#pragma once

#include<libtransistor/cpp/types.hpp>

#include "USBBridge.hpp"
#include "ELFCrashReport.hpp"

namespace twili {

class Process {
	public:
	Process(uint64_t pid);
	trn::Result<std::nullopt_t> GenerateCrashReport(ELFCrashReport &report, usb::USBBridge::USBResponseWriter &writer);

	const uint64_t pid;
	private:
};

} // namespace twili
