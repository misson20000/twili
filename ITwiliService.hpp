#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>

#include "IPipe.hpp"

namespace twili {

class ITwiliService : public Transistor::IPCServer::Object {
 public:
	ITwiliService(Transistor::IPCServer::IPCServer *server);
	
	virtual Transistor::ResultCode Dispatch(Transistor::IPC::Message msg, uint32_t request_id);
	Transistor::ResultCode OpenStdin(Transistor::IPC::OutObject<twili::IPipe> &out);
	Transistor::ResultCode OpenStdout(Transistor::IPC::OutObject<twili::IPipe> &out);
	Transistor::ResultCode OpenStderr(Transistor::IPC::OutObject<twili::IPipe> &out);
	Transistor::ResultCode Destroy();
};

}
