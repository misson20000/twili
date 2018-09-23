#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>
#include<libtransistor/loader_config.h>

namespace twili {

class IFile : public trn::ipc::server::Object {
 public:
	IFile(trn::ipc::server::IPCServer *server, std::shared_ptr<std::vector<uint8_t>> file_data);

	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id);
	trn::ResultCode Read(trn::ipc::InRaw<uint32_t> unk1, trn::ipc::InRaw<uint64_t> offset, trn::ipc::InRaw<uint64_t> size, trn::ipc::OutRaw<uint64_t> out_size, trn::ipc::Buffer<uint8_t, 0x46, 0> out_buf);
	trn::ResultCode GetSize(trn::ipc::OutRaw<uint64_t> size);

 private:
	std::shared_ptr<std::vector<uint8_t>> file_data;
};

}
