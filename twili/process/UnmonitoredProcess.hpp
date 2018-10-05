#pragma once

#include "Process.hpp"

namespace twili {
namespace process {

class UnmonitoredProcess : public Process {
 public:
	UnmonitoredProcess(Twili &twili, uint64_t pid);

	virtual uint64_t GetPid() override;
	virtual void Terminate() override;
 private:
	uint64_t pid;
};

} // namespace process
} // namespace twili
