#include<unistd.h>

#include "twili.hpp"
#include "ITwiliService.hpp"
#include "err.hpp"

namespace twili {

ITwiliService::ITwiliService(Twili *twili) : Transistor::IPCServer::Object(&twili->server), twili(twili) {
}

Transistor::ResultCode ITwiliService::Dispatch(Transistor::IPC::Message msg, uint32_t request_id) {
	switch(request_id) {
	case 0:
		return Transistor::IPCServer::RequestHandler<&ITwiliService::OpenStdin>::Handle(this, msg);
	case 1:
		return Transistor::IPCServer::RequestHandler<&ITwiliService::OpenStdout>::Handle(this, msg);
	case 2:
		return Transistor::IPCServer::RequestHandler<&ITwiliService::OpenStderr>::Handle(this, msg);
	case 3:
		return Transistor::IPCServer::RequestHandler<&ITwiliService::OpenHBABIShim>::Handle(this, msg);
	case 999:
		return Transistor::IPCServer::RequestHandler<&ITwiliService::Destroy>::Handle(this, msg);
	}
	return 1;
}

Transistor::ResultCode ITwiliService::OpenStdin(Transistor::IPC::OutObject<twili::IPipe> &val) {
	auto r = server->CreateObject<twili::IPipe>(this, STDIN_FILENO);
	if(r) {
		val.value = r.value();
		return RESULT_OK;
	} else {
		return r.error().code;
	}
}

Transistor::ResultCode ITwiliService::OpenStdout(Transistor::IPC::OutObject<twili::IPipe> &val) {
	auto r = server->CreateObject<twili::IPipe>(this, STDOUT_FILENO);
	if(r) {
		val.value = r.value();
		return RESULT_OK;
	} else {
		return r.error().code;
	}
}

Transistor::ResultCode ITwiliService::OpenStderr(Transistor::IPC::OutObject<twili::IPipe> &val) {
	auto r = server->CreateObject<twili::IPipe>(this, STDERR_FILENO);
	if(r) {
		val.value = r.value();
		return RESULT_OK;
	} else {
		return r.error().code;
	}
}

Transistor::ResultCode ITwiliService::OpenHBABIShim(Transistor::IPC::Pid pid, Transistor::IPC::OutObject<twili::IHBABIShim> &out) {
	printf("opening HBABI shim for pid 0x%x\n", pid.value);
	auto i = std::find_if(
		twili->monitored_processes.begin(),
		twili->monitored_processes.end(),
		[pid](auto const &proc) {
			return proc.pid == pid.value;
		});
	if(i == twili->monitored_processes.end()) {
		printf("couldn't find process\n");
		return TWILI_ERR_UNRECOGNIZED_PID;
	}
	auto r = server->CreateObject<twili::IHBABIShim>(this, &(*i));
	if(r) {
		out.value = r.value();
		return RESULT_OK;
	} else {
		return r.error().code;
	}
}

Transistor::ResultCode ITwiliService::Destroy() {
	twili->destroy_flag = true;
	return RESULT_OK;
}

}
