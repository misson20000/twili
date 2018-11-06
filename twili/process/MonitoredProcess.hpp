//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2018 misson20000 <xenotoad@xenotoad.net>
//
// This file is part of Twili.
//
// Twili is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Twili is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Twili.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/waiter.hpp>
#include<libtransistor/loader_config.h>

#include<memory>
#include<list>

#include "Process.hpp"
#include "../TwibPipe.hpp"
#include "../bridge/ResponseOpener.hpp"

namespace twili {

class Twili;

namespace process {

class ProcessMonitor;

class MonitoredProcess : public Process, public std::enable_shared_from_this<MonitoredProcess> {
 public:
	MonitoredProcess(Twili &twili);
	virtual ~MonitoredProcess();

	enum class State {
		Created, // process has been created, but not yet started
		Started, // process has been started, but not yet attached
		Attached, // process has been attached and is in HBABIShim
		Running, // process has entered target
		Crashed, // process has crashed
		Exited // process has exited cleanly
	};
	
	virtual void Launch(bridge::ResponseOpener response) = 0;
	virtual void AppendCode(std::vector<uint8_t> code) = 0;
	
	virtual uint64_t GetPid() override;
	virtual void AddNotes(ELFCrashReport &report) override;
	virtual void Terminate() override;
	virtual void Kill(); // terminate cleanly, if possible
	
	State GetState();
	trn::ResultCode GetResult();
	
	// these shouldn't really be public
	void Attach(std::shared_ptr<trn::KProcess> process);
	void SetResult(trn::ResultCode r);
	virtual void ChangeState(State state);
	std::shared_ptr<trn::KProcess> GetProcess(); // needed by HBABIShim
	uint64_t GetTargetEntry(); // needed by HBABIShim
	virtual void AddHBABIEntries(std::vector<loader_config_entry_t> &entries);

	std::shared_ptr<TwibPipe> tp_stdin = std::make_shared<TwibPipe>();
	std::shared_ptr<TwibPipe> tp_stdout = std::make_shared<TwibPipe>();
	std::shared_ptr<TwibPipe> tp_stderr = std::make_shared<TwibPipe>();

	std::string next_load_path;
	std::string next_load_argv;
	
	void AddMonitor(ProcessMonitor &monitor);
	void RemoveMonitor(ProcessMonitor &monitor);
	
 protected:
	std::shared_ptr<trn::KProcess> proc;
	uint64_t target_entry = 0;
	
 private:
	State state = State::Created;
	trn::ResultCode result = RESULT_OK;
	
	std::list<ProcessMonitor*> monitors;
};

} // namespace process
} // namespace twili
