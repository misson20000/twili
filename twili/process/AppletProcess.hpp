#pragma once

#include<libtransistor/cpp/nx.hpp>

#include<memory>
#include<optional>

#include "MonitoredProcess.hpp"
#include "fs/ProcessFile.hpp"
#include "fs/ProcessFileSystem.hpp"

namespace twili {

class AppletTracker;

namespace process {

class AppletProcess : public MonitoredProcess {
 public:
	AppletProcess(Twili &twili);
	
	virtual void Launch(bridge::ResponseOpener) override;
	virtual void AppendCode(std::vector<uint8_t>) override;
	virtual void AddHBABIEntries(std::vector<loader_config_entry_t> &entries) override;
	virtual void ChangeState(State state) override;
	
	// sets up ExternalContentSource for loader
	void PrepareForLaunch();
	
 private:
	bool has_code = false;
	std::optional<bridge::ResponseOpener> run_opener;
	fs::ProcessFileSystem virtual_exefs;
};

} // namespace process
} // namespace twili
