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
	
	virtual void Launch() = 0;
	void Attach(std::shared_ptr<trn::KProcess> process);
	
	virtual uint64_t GetPid() override;
	virtual void AddNotes(ELFCrashReport &report) override;
	virtual void Terminate() override;

	virtual void AddHBABIEntries(std::vector<loader_config_entry_t> &entries);
	
	std::shared_ptr<TwibPipe> tp_stdin = std::make_shared<TwibPipe>();
	std::shared_ptr<TwibPipe> tp_stdout = std::make_shared<TwibPipe>();
	std::shared_ptr<TwibPipe> tp_stderr = std::make_shared<TwibPipe>();
	
	std::shared_ptr<trn::KProcess> proc;
	uint64_t target_entry;

	bool destroy_flag = false;
	bool crashed = false;
 private:
	std::optional<bridge::ResponseOpener> attachment_opener;
};

} // namespace process
} // namespace twili
