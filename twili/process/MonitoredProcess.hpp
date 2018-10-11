#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/waiter.hpp>
#include<libtransistor/loader_config.h>

#include<memory>

#include "Process.hpp"
#include "../TwibPipe.hpp"
#include "../bridge/ResponseOpener.hpp"

namespace twili {

class Twili;

namespace process {

class MonitoredProcess : public Process, public std::enable_shared_from_this<MonitoredProcess> {
 public:
	MonitoredProcess(Twili &twili, bridge::ResponseOpener attachment_opener);
	virtual ~MonitoredProcess();

	enum class State {
		Starting, Running, Crashed, Exited
	};
	
	virtual void Launch() = 0;
	
	virtual uint64_t GetPid() override;
	virtual void AddNotes(ELFCrashReport &report) override;
	virtual void Terminate() override;

	State GetState();
	trn::ResultCode GetResult();
	
	// these shouldn't really be public
	void Attach(std::shared_ptr<trn::KProcess> process);
	void SetResult(trn::ResultCode r);
	void ChangeState(State state);
	std::shared_ptr<trn::KProcess> GetProcess(); // needed by HBABIShim
	uint64_t GetTargetEntry(); // needed by HBABIShim
	virtual void AddHBABIEntries(std::vector<loader_config_entry_t> &entries);

	std::shared_ptr<TwibPipe> tp_stdin = std::make_shared<TwibPipe>();
	std::shared_ptr<TwibPipe> tp_stdout = std::make_shared<TwibPipe>();
	std::shared_ptr<TwibPipe> tp_stderr = std::make_shared<TwibPipe>();
	
 protected:
	std::shared_ptr<trn::KProcess> proc;
	uint64_t target_entry;
	
 private:
	State state = State::Starting;
	trn::ResultCode result = RESULT_OK;
	
	std::optional<bridge::ResponseOpener> attachment_opener;
};

} // namespace process
} // namespace twili
