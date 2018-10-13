#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>

namespace twili {
namespace service {
namespace fs {

class IFile : public trn::ipc::server::Object {
 public:
	IFile(trn::ipc::server::IPCServer *server);

	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id) override;
	virtual trn::ResultCode Read(trn::ipc::InRaw<uint32_t> unk0, trn::ipc::InRaw<uint64_t> offset, trn::ipc::InRaw<uint64_t> in_size, trn::ipc::OutRaw<uint64_t> out_size, trn::ipc::Buffer<uint8_t, 0x46, 0> out_buffer) = 0;
	virtual trn::ResultCode GetSize(trn::ipc::OutRaw<uint64_t> file_size) = 0;
};

} // namespace fs
} // namespace service
} // namespace twili
