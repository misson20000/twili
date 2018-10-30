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

#include "ITwiliService.hpp"

#include<unistd.h>
#include<string.h>

#include "../twili.hpp"

#include "IPipe.hpp"
#include "IHBABIShim.hpp"
#include "IAppletShim.hpp"

#include "err.hpp"

namespace twili {
namespace service {

ITwiliService::ITwiliService(Twili *twili) : trn::ipc::server::Object(&twili->server), twili(twili) {
}

trn::ResultCode ITwiliService::Dispatch(trn::ipc::Message msg, uint32_t request_id) {
	switch(request_id) {
	case 0:
		return trn::ipc::server::RequestHandler<&ITwiliService::OpenStdin>::Handle(this, msg);
	case 1:
		return trn::ipc::server::RequestHandler<&ITwiliService::OpenStdout>::Handle(this, msg);
	case 2:
		return trn::ipc::server::RequestHandler<&ITwiliService::OpenStderr>::Handle(this, msg);
	case 3:
		return trn::ipc::server::RequestHandler<&ITwiliService::OpenHBABIShim>::Handle(this, msg);
	case 4:
		return trn::ipc::server::RequestHandler<&ITwiliService::OpenAppletShim>::Handle(this, msg);
	case 10:
		return trn::ipc::server::RequestHandler<&ITwiliService::CreateNamedOutputPipe>::Handle(this, msg);
	case 999:
		return trn::ipc::server::RequestHandler<&ITwiliService::Destroy>::Handle(this, msg);
	}
	return 1;
}

trn::ResultCode ITwiliService::OpenPipe(trn::ipc::InPid pid, trn::ipc::OutObject<IPipe> &val, int fd) {
	IPipe *object;
	std::shared_ptr<process::MonitoredProcess> proc = twili->FindMonitoredProcess(pid.value);
	if(!proc) {
		printf("opening pipe %d for non-monitored process: %ld\n", fd, pid.value);
		object = trn::ResultCode::AssertOk(server->CreateObject<IPipeStandard>(this, fd));
	} else {
		printf("opening pipe %d for monitored process: %ld\n", fd, pid.value);
		std::shared_ptr<TwibPipe> pipe;
		switch(fd) {
		case 0:
			pipe = proc->tp_stdin;
			break;
		case 1:
			pipe = proc->tp_stdout;
			break;
		case 2:
			pipe = proc->tp_stderr;
			break;
		default:
			return TWILI_ERR_INVALID_PIPE;
		}
		object = trn::ResultCode::AssertOk(server->CreateObject<IPipeTwib>(this, pipe));
	}
	val.value = object;
	return RESULT_OK;
}

trn::ResultCode ITwiliService::OpenStdin(trn::ipc::InPid pid, trn::ipc::OutObject<IPipe> &val) {
	return OpenPipe(pid, val, STDIN_FILENO);
}

trn::ResultCode ITwiliService::OpenStdout(trn::ipc::InPid pid, trn::ipc::OutObject<IPipe> &val) {
	return OpenPipe(pid, val, STDOUT_FILENO);
}

trn::ResultCode ITwiliService::OpenStderr(trn::ipc::InPid pid, trn::ipc::OutObject<IPipe> &val) {
	return OpenPipe(pid, val, STDERR_FILENO);
}

trn::ResultCode ITwiliService::OpenHBABIShim(trn::ipc::InPid pid, trn::ipc::OutObject<IHBABIShim> &out) {
	printf("opening HBABI shim for pid 0x%lx\n", pid.value);
	std::shared_ptr<process::MonitoredProcess> i = twili->FindMonitoredProcess(pid.value);
	if(!i) {
		printf("couldn't find process\n");
		return TWILI_ERR_UNRECOGNIZED_PID;
	}
	auto r = server->CreateObject<IHBABIShim>(this, i);
	if(r) {
		out.value = r.value();
		return RESULT_OK;
	} else {
		return r.error().code;
	}
}

trn::ResultCode ITwiliService::OpenAppletShim(trn::ipc::InPid pid, trn::ipc::InHandle<trn::KProcess, trn::ipc::copy> process, trn::ipc::OutObject<IAppletShim> &out) {
	printf("opening applet shim for pid 0x%lx\n", pid.value);

	if(!twili->applet_tracker.HasControlProcess()) {
		printf("creating control shim\n");
		auto r = server->CreateObject<IAppletShim::ControlImpl>(this, twili->applet_tracker);
		if(r) {
			out.value = r.value();
			return RESULT_OK;
		} else {
			return r.error().code;
		}
	} else {
		printf("attaching host shim\n");
		auto r = server->CreateObject<IAppletShim::HostImpl>(this, twili->applet_tracker.AttachHostProcess(std::move(process.object)));
		if(r) {
			out.value = r.value();
			return RESULT_OK;
		} else {
			return r.error().code;
		}
	}
}

trn::ResultCode ITwiliService::CreateNamedOutputPipe(trn::ipc::Buffer<uint8_t, 0x5, 0> name_buffer, trn::ipc::OutObject<IPipe> &val) {
	std::string name((char*) name_buffer.data, (char*) name_buffer.data + strnlen((char*) name_buffer.data, name_buffer.size));

	auto r = twili->named_pipes.emplace(name, std::make_shared<TwibPipe>());
	if(r.second) {
		val.value = trn::ResultCode::AssertOk(server->CreateObject<IPipeTwib>(this, r.first->second));
		return RESULT_OK;
	} else {
		return TWILI_ERR_PIPE_ALREADY_EXISTS;
	}
}

trn::ResultCode ITwiliService::Destroy() {
	twili->destroy_flag = true;
	return RESULT_OK;
}

} // namespace service
} // namespace twili
