#include<unistd.h>

#include "ITwiliService.hpp"

namespace twili {

ITwiliService::ITwiliService(Transistor::IPCServer::IPCServer *server) : Transistor::IPCServer::Object(server) {
}

Transistor::ResultCode ITwiliService::Dispatch(Transistor::IPC::Message msg, uint32_t request_id) {
	switch(request_id) {
	case 0:
		return Transistor::IPCServer::RequestHandler<&ITwiliService::OpenStdin>::Handle(this, msg);
	case 1:
		return Transistor::IPCServer::RequestHandler<&ITwiliService::OpenStdout>::Handle(this, msg);
	case 2:
		return Transistor::IPCServer::RequestHandler<&ITwiliService::OpenStderr>::Handle(this, msg);
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

}
