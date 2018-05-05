#include<unistd.h>

#include "twili.hpp"
#include "ITwiliService.hpp"
#include "err.hpp"

namespace twili {

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
	case 999:
		return trn::ipc::server::RequestHandler<&ITwiliService::Destroy>::Handle(this, msg);
	}
	return 1;
}

trn::ResultCode ITwiliService::OpenStdin(trn::ipc::OutObject<twili::IPipe> &val) {
	auto r = server->CreateObject<twili::IPipe>(this, STDIN_FILENO);
	if(r) {
		val.value = r.value();
		return RESULT_OK;
	} else {
		return r.error().code;
	}
}

trn::ResultCode ITwiliService::OpenStdout(trn::ipc::OutObject<twili::IPipe> &val) {
	auto r = server->CreateObject<twili::IPipe>(this, STDOUT_FILENO);
	if(r) {
		val.value = r.value();
		return RESULT_OK;
	} else {
		return r.error().code;
	}
}

trn::ResultCode ITwiliService::OpenStderr(trn::ipc::OutObject<twili::IPipe> &val) {
	auto r = server->CreateObject<twili::IPipe>(this, STDERR_FILENO);
	if(r) {
		val.value = r.value();
		return RESULT_OK;
	} else {
		return r.error().code;
	}
}

trn::ResultCode ITwiliService::OpenHBABIShim(trn::ipc::InPid pid, trn::ipc::OutObject<twili::IHBABIShim> &out) {
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

trn::ResultCode ITwiliService::Destroy() {
	twili->destroy_flag = true;
	return RESULT_OK;
}

}
