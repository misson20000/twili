#pragma once

#include<libtransistor/cpp/types.hpp>

#include "bridge/ResponseOpener.hpp"
#include "ELFCrashReport.hpp"

namespace twili {

class Twili;

class Process {
 public:
	Process(Twili &twili, uint64_t pid);
	void GenerateCrashReport(ELFCrashReport &report, bridge::ResponseOpener opener);

	const uint64_t pid;
 private:
	Twili &twili;
};

} // namespace twili
