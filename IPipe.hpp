#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>

namespace twili {

class IPipe : public Transistor::IPCServer::Object {
 public:
	IPipe(Transistor::IPCServer::IPCServer *server, int fd);
	virtual Transistor::ResultCode Dispatch(Transistor::IPC::Message msg, uint32_t request_id);

	Transistor::ResultCode Read(Transistor::IPC::OutRaw<uint64_t> size, Transistor::IPC::Buffer<uint8_t, 0x6, 0> buffer);
	Transistor::ResultCode Write(Transistor::IPC::Buffer<uint8_t, 0x5, 0> buffer);
 private:
	int fd;
};

}
