#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>
#include<libtransistor/loader_config.h>

namespace twili {

class MonitoredProcess;

class IHBABIShim : public Transistor::IPCServer::Object {
 public:
	IHBABIShim(Transistor::IPCServer::IPCServer *server, MonitoredProcess *process);
	
	virtual Transistor::ResultCode Dispatch(Transistor::IPC::Message msg, uint32_t request_id);
	Transistor::ResultCode GetProcessHandle(Transistor::IPC::OutHandle<Transistor::KProcess, Transistor::IPC::copy> out);
	Transistor::ResultCode GetLoaderConfigEntryCount(Transistor::IPC::OutRaw<uint32_t> out);
	Transistor::ResultCode GetLoaderConfigEntries(Transistor::IPC::Buffer<loader_config_entry_t, 0x6, 0> buffer);
	Transistor::ResultCode GetLoaderConfigHandle(Transistor::IPC::InRaw<uint32_t> placeholder, Transistor::IPC::OutHandle<handle_t, Transistor::IPC::copy> out);
	Transistor::ResultCode SetNextLoadPath(Transistor::IPC::Buffer<uint8_t, 0x5, 0> path, Transistor::IPC::Buffer<uint8_t, 0x5, 0> argv);
	Transistor::ResultCode GetTargetEntryPoint(Transistor::IPC::OutRaw<uint64_t> out);
	Transistor::ResultCode SetExitCode(Transistor::IPC::InRaw<uint32_t> code);
 private:
	MonitoredProcess *process;
	std::vector<loader_config_entry_t> entries;
	std::vector<Transistor::KObject*> handles;
};

}
