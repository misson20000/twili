#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>

#include "IPipe.hpp"
#include "IHBABIShim.hpp"

namespace twili {

class ITwiliService : public trn::ipc::server::Object {
 public:
	ITwiliService(Twili *twili);
	
	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id);
	trn::ResultCode OpenStdin(trn::ipc::InPid pid, trn::ipc::OutObject<twili::IPipe> &out);
	trn::ResultCode OpenStdout(trn::ipc::InPid pid, trn::ipc::OutObject<twili::IPipe> &out);
	trn::ResultCode OpenStderr(trn::ipc::InPid pid, trn::ipc::OutObject<twili::IPipe> &out);
	trn::ResultCode OpenHBABIShim(trn::ipc::InPid pid, trn::ipc::OutObject<twili::IHBABIShim> &out);
	trn::ResultCode Destroy();

  private:
   trn::ResultCode OpenPipe(trn::ipc::InPid pid, trn::ipc::OutObject<twili::IPipe> &out, int fd);
 private:
	Twili *twili;
};

}
