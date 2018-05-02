#include "twili.hpp"
#include "IHBABIShim.hpp"
#include "MonitoredProcess.hpp"
#include "err.hpp"

namespace twili {

IHBABIShim::IHBABIShim(Transistor::IPCServer::IPCServer *server, MonitoredProcess *process) : Transistor::IPCServer::Object(server), process(process) {
	printf("opened HBABI shim for 0x%x\n", process->proc->handle);
}

Transistor::ResultCode IHBABIShim::Dispatch(Transistor::IPC::Message msg, uint32_t request_id) {
	switch(request_id) {
	case 0:
		return Transistor::IPCServer::RequestHandler<&IHBABIShim::GetProcessHandle>::Handle(this, msg);
	case 1:
		return Transistor::IPCServer::RequestHandler<&IHBABIShim::GetLoaderConfigEntryCount>::Handle(this, msg);
	case 2:
		return Transistor::IPCServer::RequestHandler<&IHBABIShim::GetLoaderConfigEntries>::Handle(this, msg);
	case 3:
		return Transistor::IPCServer::RequestHandler<&IHBABIShim::GetLoaderConfigHandle>::Handle(this, msg);
	case 4:
		return Transistor::IPCServer::RequestHandler<&IHBABIShim::SetNextLoadPath>::Handle(this, msg);
	case 5:
		return Transistor::IPCServer::RequestHandler<&IHBABIShim::GetTargetEntryPoint>::Handle(this, msg);
	case 6:
		return Transistor::IPCServer::RequestHandler<&IHBABIShim::SetExitCode>::Handle(this, msg);
	}
	return 1;
}

Transistor::ResultCode IHBABIShim::GetProcessHandle(Transistor::IPC::OutHandle<Transistor::KProcess, Transistor::IPC::copy> out) {
	out = *process->proc;
	return RESULT_OK;
}

Transistor::ResultCode IHBABIShim::GetLoaderConfigEntryCount(Transistor::IPC::OutRaw<uint32_t> out) {
	out = entries.size();
	return RESULT_OK;
}

Transistor::ResultCode IHBABIShim::GetLoaderConfigEntries(Transistor::IPC::Buffer<loader_config_entry_t, 0x6, 0> buffer) {
	std::copy(entries.begin(), entries.begin() + (buffer.size / sizeof(loader_config_entry_t)), buffer.data);
	return RESULT_OK;
}

Transistor::ResultCode IHBABIShim::GetLoaderConfigHandle(Transistor::IPC::InRaw<uint32_t> placeholder, Transistor::IPC::OutHandle<handle_t, Transistor::IPC::copy> out) {
	if(placeholder.value > handles.size()) {
		return TWILI_ERR_UNRECOGNIZED_HANDLE_PLACEHOLDER;
	}
	out = handles[*placeholder]->handle;
	return RESULT_OK;
}

Transistor::ResultCode IHBABIShim::SetNextLoadPath(Transistor::IPC::Buffer<uint8_t, 0x5, 0> path, Transistor::IPC::Buffer<uint8_t, 0x5, 0> argv) {
	printf("[HBABIShim(0x%x)] next load path: %s[%s]\n", process->proc->handle, path.data, argv.data);
	return RESULT_OK;
}

Transistor::ResultCode IHBABIShim::GetTargetEntryPoint(Transistor::IPC::OutRaw<uint64_t> out) {
	out = process->target_entry;
	return RESULT_OK;
}

Transistor::ResultCode IHBABIShim::SetExitCode(Transistor::IPC::InRaw<uint32_t> code) {
	printf("[HBABIShim(0x%x)] exit code 0x%x\n", process->proc->handle, code.value);
	return RESULT_OK;
}

}
