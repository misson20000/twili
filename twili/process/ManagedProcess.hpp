#pragma once

#include "MonitoredProcess.hpp"

#include "../process_creation.hpp"

namespace twili {
namespace process {

class ManagedProcess : public MonitoredProcess {
 public:
	ManagedProcess(Twili &twili);
	virtual void Launch(bridge::ResponseOpener response) override;
	virtual void AppendCode(std::vector<uint8_t> nro) override;
 private:
	process_creation::ProcessBuilder builder;
	process_creation::ProcessBuilder::VectorDataReader hbabi_shim_reader;
	std::vector<process_creation::ProcessBuilder::VectorDataReader> readers;
	
	std::shared_ptr<trn::WaitHandle> wait;
};

} // namespace process
} // namespace twili
