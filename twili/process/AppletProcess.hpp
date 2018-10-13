#pragma once

#include<libtransistor/cpp/nx.hpp>

#include<memory>

#include "MonitoredProcess.hpp"
#include "fs/ProcessFile.hpp"
#include "fs/ProcessFileSystem.hpp"

namespace twili {

class AppletTracker;

namespace process {

class AppletProcess : public MonitoredProcess {
 public:
	AppletProcess(Twili &twili, bridge::ResponseOpener attachment_opener, std::vector<uint8_t> nso);
	
	virtual void Launch() override;
	virtual void AddHBABIEntries(std::vector<loader_config_entry_t> &entries) override;

	// sets up ExternalContentSource for loader
	void PrepareForLaunch();
	
 private:
	fs::ProcessFileSystem virtual_exefs;
};

} // namespace process
} // namespace twili
