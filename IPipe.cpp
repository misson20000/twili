#include<unistd.h>

#include "IPipe.hpp"

namespace twili {

IPipe::IPipe(Transistor::IPCServer::IPCServer *server, int fd) : Transistor::IPCServer::Object(server), fd(fd) {
}

Transistor::ResultCode IPipe::Dispatch(Transistor::IPC::Message msg, uint32_t request_id) {
	switch(request_id) {
	case 0:
		return Transistor::IPCServer::RequestHandler<&IPipe::Read>::Handle(this, msg);
	case 1:
		return Transistor::IPCServer::RequestHandler<&IPipe::Write>::Handle(this, msg);
	}
	return 1;
}

Transistor::ResultCode IPipe::Read(Transistor::IPC::OutRaw<uint64_t> size, Transistor::IPC::Buffer<uint8_t, 0x6, 0> buffer) {
	size = read(fd, buffer.data, buffer.size);
	return RESULT_OK;
}

Transistor::ResultCode IPipe::Write(Transistor::IPC::Buffer<uint8_t, 0x5, 0> buffer) {
	printf("got write\n");
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
