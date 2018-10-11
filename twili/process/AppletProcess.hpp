#pragma once

#include<libtransistor/cpp/nx.hpp>

#include<memory>

#include "MonitoredProcess.hpp"

#include "../process_creation.hpp"

namespace twili {

class AppletTracker;

namespace process {

class AppletProcess : public MonitoredProcess {
 public:
	AppletProcess(Twili &twili, bridge::ResponseOpener attachment_opener, std::vector<uint8_t> nro);
	
	virtual void Launch() override;
	virtual void AddHBABIEntries(std::vector<loader_config_entry_t> &entries) override;
	
	size_t GetTargetSize();
	void SetupTarget();
	
 private:
	process_creation::ProcessBuilder::VectorDataReader reader;
	process_creation::ProcessBuilder builder;
};

} // namespace process
} // namespace twili
