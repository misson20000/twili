#pragma once

#include "MonitoredProcess.hpp"

namespace twili {
namespace process {

class ManagedProcess : public MonitoredProcess {
 public:
	ManagedProcess(Twili &twili, std::vector<uint8_t> nro);
	virtual void Launch() override;
 private:
	std::shared_ptr<trn::WaitHandle> wait;
};

} // namespace process
} // namespace twili
