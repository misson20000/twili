#include<unistd.h>

#include "IPipe.hpp"

namespace twili {

IPipe::IPipe(trn::ipc::server::IPCServer *server, int fd, MonitoredProcess *proc) :
   trn::ipc::server::Object(server),
   fd(fd),
   proc(proc) {
}

trn::ResultCode IPipe::Dispatch(trn::ipc::Message msg, uint32_t request_id) {
	switch(request_id) {
	case 0:
		return trn::ipc::server::RequestHandler<&IPipe::Read>::Handle(this, msg);
	case 1:
		return trn::ipc::server::RequestHandler<&IPipe::Write>::Handle(this, msg);
	}
	return 1;
}

trn::ResultCode IPipe::Read(trn::ipc::OutRaw<uint64_t> size, trn::ipc::Buffer<uint8_t, 0x6, 0> buffer) {
	size = read(fd, buffer.data, buffer.size);
	return RESULT_OK;
}

trn::ResultCode IPipe::Write(trn::ipc::Buffer<uint8_t, 0x5, 0> buffer) {
	size_t written = 0;
	while(written < buffer.size) {
		ssize_t ret = write(fd, buffer.data + written, buffer.size - written);
		if(ret <= 0) {
			return 1;
		}
		written+= ret;
	}
	return RESULT_OK;
}

}
