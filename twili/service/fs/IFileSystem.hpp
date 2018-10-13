#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>

namespace twili {
namespace service {
namespace fs {

class IFile;

class IFileSystem : public trn::ipc::server::Object {
 public:
	IFileSystem(trn::ipc::server::IPCServer *server);

	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id) override;
	virtual trn::ResultCode OpenFile(trn::ipc::InRaw<uint32_t> mode, trn::ipc::Buffer<char, 0x19, 0> path, trn::ipc::OutObject<IFile> &out) = 0;
};

} // namespace fs
} // namespace service
} // namespace twili
