#pragma once

#include<libtransistor/cpp/types.hpp>

#include "USBBridge.hpp"
#include "ELFCrashReport.hpp"

namespace twili {

class Process {
 public:
	Process(Twili &twili, uint64_t pid);
	void GenerateCrashReport(ELFCrashReport &report, usb::USBBridge::ResponseOpener opener);

	const uint64_t pid;
 private:
	Twili &twili;
};

} // namespace twili
