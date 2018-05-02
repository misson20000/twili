#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>

#include "IPipe.hpp"
#include "IHBABIShim.hpp"

namespace twili {

class ITwiliService : public Transistor::IPCServer::Object {
 public:
	ITwiliService(Twili *twili);
	
	virtual Transistor::ResultCode Dispatch(Transistor::IPC::Message msg, uint32_t request_id);
	Transistor::ResultCode OpenStdin(Transistor::IPC::OutObject<twili::IPipe> &out);
	Transistor::ResultCode OpenStdout(Transistor::IPC::OutObject<twili::IPipe> &out);
	Transistor::ResultCode OpenStderr(Transistor::IPC::OutObject<twili::IPipe> &out);
	Transistor::ResultCode OpenHBABIShim(Transistor::IPC::Pid pid, Transistor::IPC::OutObject<twili::IHBABIShim> &out);
	Transistor::ResultCode Destroy();
 private:
	Twili *twili;
};

}
