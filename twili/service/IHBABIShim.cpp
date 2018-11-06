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

#include "IHBABIShim.hpp"

#include "../twili.hpp"
#include "../process/MonitoredProcess.hpp"

#include "err.hpp"

namespace twili {
namespace service {

IHBABIShim::IHBABIShim(trn::ipc::server::IPCServer *server, std::shared_ptr<process::MonitoredProcess> process) : trn::ipc::server::Object(server), process(process) {
	process->AddHBABIEntries(entries);
}

trn::ResultCode IHBABIShim::Dispatch(trn::ipc::Message msg, uint32_t request_id) {
	switch(request_id) {
	case 0:
		return trn::ipc::server::RequestHandler<&IHBABIShim::GetProcessHandle>::Handle(this, msg);
	case 1:
		return trn::ipc::server::RequestHandler<&IHBABIShim::GetLoaderConfigEntryCount>::Handle(this, msg);
	case 2:
		return trn::ipc::server::RequestHandler<&IHBABIShim::GetLoaderConfigEntries>::Handle(this, msg);
	case 3:
		return trn::ipc::server::RequestHandler<&IHBABIShim::GetLoaderConfigHandle>::Handle(this, msg);
	case 4:
		return trn::ipc::server::RequestHandler<&IHBABIShim::SetNextLoadPath>::Handle(this, msg);
	case 5:
		return trn::ipc::server::RequestHandler<&IHBABIShim::GetTargetEntryPoint>::Handle(this, msg);
	case 6:
		return trn::ipc::server::RequestHandler<&IHBABIShim::SetExitCode>::Handle(this, msg);
	case 7:
		return trn::ipc::server::RequestHandler<&IHBABIShim::WaitToStart>::Handle(this, msg);
	}
	return 1;
}

trn::ResultCode IHBABIShim::GetProcessHandle(trn::ipc::OutHandle<handle_t, trn::ipc::copy> out) {
	out = process->GetProcess()->handle;
	return RESULT_OK;
}

trn::ResultCode IHBABIShim::GetLoaderConfigEntryCount(trn::ipc::OutRaw<uint32_t> out) {
	out = entries.size();
	return RESULT_OK;
}

trn::ResultCode IHBABIShim::GetLoaderConfigEntries(trn::ipc::Buffer<loader_config_entry_t, 0x6, 0> buffer) {
	size_t count = buffer.size / sizeof(loader_config_entry_t);
	if(count > entries.size()) {
		count = entries.size();
	}
	std::copy_n(entries.begin(), count, buffer.data);
	return RESULT_OK;
}

trn::ResultCode IHBABIShim::GetLoaderConfigHandle(trn::ipc::InRaw<uint32_t> placeholder, trn::ipc::OutHandle<handle_t, trn::ipc::copy> out) {
	if(placeholder.value > handles.size()) {
		return TWILI_ERR_UNRECOGNIZED_HANDLE_PLACEHOLDER;
	}
	out = handles[*placeholder]->handle;
	return RESULT_OK;
}

trn::ResultCode IHBABIShim::SetNextLoadPath(trn::ipc::Buffer<uint8_t, 0x5, 0> path, trn::ipc::Buffer<uint8_t, 0x5, 0> argv) {
	printf("[HBABIShim(0x%lx)] next load path: %s[%s]\n", process->GetPid(), path.data, argv.data);
	process->next_load_path = (const char*) path.data;
	process->next_load_argv = (const char*) argv.data;
	return RESULT_OK;
}

trn::ResultCode IHBABIShim::GetTargetEntryPoint(trn::ipc::OutRaw<uint64_t> out) {
	out = process->GetTargetEntry();
	return RESULT_OK;
}

trn::ResultCode IHBABIShim::SetExitCode(trn::ipc::InRaw<uint32_t> code) {
	process->SetResult(code.value);
	printf("[HBABIShim(0x%lx)] exit code 0x%x\n", process->GetPid(), code.value);
	return RESULT_OK;
}

trn::ResultCode IHBABIShim::WaitToStart(std::function<void(trn::ResultCode)> cb) {
	process->ChangeState(process::MonitoredProcess::State::Running);
	cb(RESULT_OK);
	return RESULT_OK;
}

} // namespace service
} // namespace twili
