#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>

#include "../twili.hpp"
#include "IPipe.hpp"
#include "IHBABIShim.hpp"

namespace twili {
namespace service {

class ITwiliService : public trn::ipc::server::Object {
 public:
	ITwiliService(Twili *twili);
	
	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id);
	trn::ResultCode OpenStdin(trn::ipc::InPid pid, trn::ipc::OutObject<IPipe> &out);
	trn::ResultCode OpenStdout(trn::ipc::InPid pid, trn::ipc::OutObject<IPipe> &out);
	trn::ResultCode OpenStderr(trn::ipc::InPid pid, trn::ipc::OutObject<IPipe> &out);
	trn::ResultCode OpenHBABIShim(trn::ipc::InPid pid, trn::ipc::OutObject<IHBABIShim> &out);
	trn::ResultCode CreateNamedOutputPipe(trn::ipc::Buffer<uint8_t, 0x5, 0> name_buffer, trn::ipc::OutObject<IPipe> &val);
	trn::ResultCode Destroy();

  private:
   trn::ResultCode OpenPipe(trn::ipc::InPid pid, trn::ipc::OutObject<IPipe> &out, int fd);
 private:
	Twili *twili;
};

} // namespace service
} // namespace twili
