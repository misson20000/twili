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
#include<libtransistor/cpp/ipcserver.hpp>
#include<libtransistor/loader_config.h>

namespace twili {

namespace process {
class MonitoredProcess;
}

namespace service {

class IHBABIShim : public trn::ipc::server::Object {
 public:
	IHBABIShim(trn::ipc::server::IPCServer *server, std::shared_ptr<process::MonitoredProcess> process);
	
	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id);
	trn::ResultCode GetProcessHandle(trn::ipc::OutHandle<handle_t, trn::ipc::copy> out);
	trn::ResultCode GetLoaderConfigEntryCount(trn::ipc::OutRaw<uint32_t> out);
	trn::ResultCode GetLoaderConfigEntries(trn::ipc::Buffer<loader_config_entry_t, 0x6, 0> buffer);
	trn::ResultCode GetLoaderConfigHandle(trn::ipc::InRaw<uint32_t> placeholder, trn::ipc::OutHandle<handle_t, trn::ipc::copy> out);
	trn::ResultCode SetNextLoadPath(trn::ipc::Buffer<uint8_t, 0x5, 0> path, trn::ipc::Buffer<uint8_t, 0x5, 0> argv);
	trn::ResultCode GetTargetEntryPoint(trn::ipc::OutRaw<uint64_t> out);
	trn::ResultCode SetExitCode(trn::ipc::InRaw<uint32_t> code);
	trn::ResultCode WaitToStart(std::function<void(trn::ResultCode)> cb);
 private:
	std::shared_ptr<process::MonitoredProcess> process;
	std::vector<loader_config_entry_t> entries;
	std::vector<trn::KObject*> handles;
};

} // namespace service
} // namespace twili
