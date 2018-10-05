#pragma once

#include<libtransistor/cpp/nx.hpp>

#include<memory>

#include "MonitoredProcess.hpp"

#include "process_creation.hpp"

namespace twili {

class AppletTracker;

namespace process {

class AppletProcess : public MonitoredProcess, public std::enable_shared_from_this<AppletProcess> {
 public:
	AppletProcess(Twili &twili, std::vector<uint8_t> nro);
	
	virtual void Launch() override;
	
	void Attach(trn::KProcess &&process);
	size_t GetTargetSize();
	void SetupTarget(uint64_t buffer_address, uint64_t map_address);
	void FinalizeTarget();
	
 private:
	process_creation::ProcessBuilder::VectorDataReader reader;
	process_creation::ProcessBuilder builder;
};

} // namespace process
} // namespace twili
