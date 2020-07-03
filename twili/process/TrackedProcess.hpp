//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2019 misson20000 <xenotoad@xenotoad.net>
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

#include<libtransistor/cpp/nx.hpp>

#include<memory>
#include<optional>
#include<deque>

#include "ECSProcess.hpp"
#include "fs/ProcessFile.hpp"
#include "fs/ProcessFileSystem.hpp"

namespace twili {
namespace process {

class TrackerBase {
 public:
	virtual std::shared_ptr<MonitoredProcess> CreateMonitoredProcess() = 0;
};

template<typename T>
class Tracker : public TrackerBase {
 public:
	virtual std::shared_ptr<MonitoredProcess> CreateMonitoredProcess() override {
		return CreateProcess();
	}
	virtual std::shared_ptr<T> CreateProcess() = 0;
	virtual void QueueLaunch(std::shared_ptr<T> process) = 0;
};

class TrackedProcessBase : public ECSProcess {
 public:
	TrackedProcessBase(Twili &twili, const char *rtld, const char *npdm, uint64_t title_id);
	
	virtual void ChangeState(State state) override;
	virtual void Kill() override;

	// sets up ExternalContentSource for loader,
	// may return TWILI_ERR_NO_LONGER_REQUESTED_TO_LAUNCH to cancel cleanly.
	virtual trn::ResultCode PrepareForLaunch() override;
 protected:
	std::optional<bridge::ResponseOpener> run_opener;
	virtual void KillImpl();

 private:
	bool exit_pending = false;
};

template<typename T>
class TrackedProcess : public TrackedProcessBase {
 public:
	TrackedProcess(
		Twili &twili,
		Tracker<T> &tracker,
		const char *rtld,
		const char *npdm,
		uint64_t title_id) :
		TrackedProcessBase(twili, rtld, npdm, title_id),
		tracker(tracker) {
	}

	void Launch(bridge::ResponseOpener opener) {
		run_opener = opener;
		ChangeState(State::Started);
	
		// this pointer cast is not cool.
		tracker.QueueLaunch(std::dynamic_pointer_cast<T>(shared_from_this()));
	}
	
 protected:
	Tracker<T> &tracker;
};

} // namespace process
} // namespace twili
