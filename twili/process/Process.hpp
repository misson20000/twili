#pragma once

#include<stdint.h>

namespace twili {

class Twili;
class ELFCrashReport;

namespace process {

class Process {
 public:
	Process(Twili &twili);
	virtual ~Process();

	virtual uint64_t GetPid() = 0;
	virtual void AddNotes(ELFCrashReport &report);
	virtual void Terminate() = 0;
 protected:
	Twili &twili;
};

} // namespace process
} // namespace twili
