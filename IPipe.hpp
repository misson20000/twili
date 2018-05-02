#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>

namespace twili {

class IPipe : public trn::ipc::server::Object {
 public:
	IPipe(trn::ipc::server::IPCServer *server, int fd);
	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id);

	trn::ResultCode Read(trn::ipc::OutRaw<uint64_t> size, trn::ipc::Buffer<uint8_t, 0x6, 0> buffer);
	trn::ResultCode Write(trn::ipc::Buffer<uint8_t, 0x5, 0> buffer);
 private:
	int fd;
};

}
