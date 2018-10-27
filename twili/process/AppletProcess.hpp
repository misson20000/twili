#pragma once

#include<libtransistor/cpp/nx.hpp>

#include<memory>
#include<optional>
#include<deque>

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
	virtual void Kill() override;
	
	// sets up ExternalContentSource for loader
	void PrepareForLaunch();

	// used to communicate with host shim
	trn::KEvent &GetCommandEvent();
	uint32_t PopCommand();
 private:
	bool has_code = false;
	std::optional<bridge::ResponseOpener> run_opener;
	fs::ProcessFileSystem virtual_exefs;

	std::shared_ptr<trn::WaitHandle> kill_timeout;

	void PushCommand(uint32_t command);
	trn::KEvent command_event;
	trn::KWEvent command_wevent;
	std::deque<uint32_t> commands;
};

} // namespace process
} // namespace twili
