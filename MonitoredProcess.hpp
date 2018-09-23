#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/waiter.hpp>

#include<memory>

#include "Process.hpp"
#include "ELFCrashReport.hpp"
#include "TwibPipe.hpp"

namespace twili {

class Twili;

class MonitoredProcess : public Process {
 public:
	MonitoredProcess(Twili *twili, std::shared_ptr<trn::KProcess> proc, uint64_t target_entry, std::vector<uint8_t> nro);
	
	void Launch();
	void GenerateCrashReport(ELFCrashReport &report, bridge::ResponseOpener r);
	void Terminate();

	std::shared_ptr<TwibPipe> tp_stdin = std::make_shared<TwibPipe>();
	std::shared_ptr<TwibPipe> tp_stdout = std::make_shared<TwibPipe>();
	std::shared_ptr<TwibPipe> tp_stderr = std::make_shared<TwibPipe>();
	
	std::shared_ptr<trn::KProcess> proc;
	std::shared_ptr<std::vector<uint8_t>> nro;
	const uint64_t target_entry;
	bool destroy_flag = false;
	bool crashed = false;
	
	~MonitoredProcess();
 private:
	Twili *twili;
	std::shared_ptr<trn::WaitHandle> wait;
};

}
