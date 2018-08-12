#include<unistd.h>
#include<string.h>

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
	case 10:
		return trn::ipc::server::RequestHandler<&ITwiliService::CreateNamedOutputPipe>::Handle(this, msg);
	case 999:
		return trn::ipc::server::RequestHandler<&ITwiliService::Destroy>::Handle(this, msg);
	}
	return 1;
}

trn::ResultCode ITwiliService::OpenPipe(trn::ipc::InPid pid, trn::ipc::OutObject<twili::IPipe> &val, int fd) {
	twili::IPipe *object;
	auto proc = twili->FindMonitoredProcess(pid.value);
	if(!proc) {
		printf("opening pipe %d for non-monitored process: %ld\n", fd, pid.value);
		object = trn::ResultCode::AssertOk(server->CreateObject<twili::IPipeStandard>(this, fd));
	} else {
		printf("opening pipe %d for monitored process: %ld\n", fd, pid.value);
		std::shared_ptr<TwibPipe> pipe;
		switch(fd) {
		case 0:
			pipe = (*proc)->tp_stdin;
			break;
		case 1:
			pipe = (*proc)->tp_stdout;
			break;
		case 2:
			pipe = (*proc)->tp_stderr;
			break;
		default:
			return TWILI_ERR_INVALID_PIPE;
		}
		object = trn::ResultCode::AssertOk(server->CreateObject<twili::IPipeTwib>(this, pipe));
	}
	val.value = object;
	return RESULT_OK;
}

trn::ResultCode ITwiliService::OpenStdin(trn::ipc::InPid pid, trn::ipc::OutObject<twili::IPipe> &val) {
	return OpenPipe(pid, val, STDIN_FILENO);
}

trn::ResultCode ITwiliService::OpenStdout(trn::ipc::InPid pid, trn::ipc::OutObject<twili::IPipe> &val) {
	return OpenPipe(pid, val, STDOUT_FILENO);
}

trn::ResultCode ITwiliService::OpenStderr(trn::ipc::InPid pid, trn::ipc::OutObject<twili::IPipe> &val) {
	return OpenPipe(pid, val, STDERR_FILENO);
}

trn::ResultCode ITwiliService::OpenHBABIShim(trn::ipc::InPid pid, trn::ipc::OutObject<twili::IHBABIShim> &out) {
	printf("opening HBABI shim for pid 0x%lx\n", pid.value);
   auto i = twili->FindMonitoredProcess(pid.value);
	if(!i) {
		printf("couldn't find process\n");
		return TWILI_ERR_UNRECOGNIZED_PID;
	}
	auto r = server->CreateObject<twili::IHBABIShim>(this, *i);
	if(r) {
		out.value = r.value();
		return RESULT_OK;
	} else {
		return r.error().code;
	}
}

trn::ResultCode ITwiliService::CreateNamedOutputPipe(trn::ipc::Buffer<uint8_t, 0x5, 0> name_buffer, trn::ipc::OutObject<twili::IPipe> &val) {
	std::string name((char*) name_buffer.data, (char*) name_buffer.data + strnlen((char*) name_buffer.data, name_buffer.size));

	auto r = twili->named_pipes.emplace(name, std::make_shared<TwibPipe>());
	if(r.second) {
		val.value = trn::ResultCode::AssertOk(server->CreateObject<twili::IPipeTwib>(this, r.first->second));
		return RESULT_OK;
	} else {
		return TWILI_ERR_PIPE_ALREADY_EXISTS;
	}
}

trn::ResultCode ITwiliService::Destroy() {
	twili->destroy_flag = true;
	return RESULT_OK;
}

}
