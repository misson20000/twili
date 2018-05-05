#include "twili.hpp"
#include "IHBABIShim.hpp"
#include "MonitoredProcess.hpp"
#include "err.hpp"

namespace twili {

IHBABIShim::IHBABIShim(trn::ipc::server::IPCServer *server, MonitoredProcess *process) : trn::ipc::server::Object(server), process(process) {
	printf("opened HBABI shim for 0x%x\n", process->proc->handle);
	entries.push_back({
		.key = LCONFIG_KEY_TWILI_PRESENT,
		.flags = 0,
	});
	entries.push_back({
		.key = LCONFIG_KEY_SYSCALL_AVAILABLE_HINT,
		.flags = 0,
		.syscall_available_hint = {
			0xffffffffffffffff,
			0xffffffffffffffff
		}
	});
}

trn::ResultCode IHBABIShim::Dispatch(trn::ipc::Message msg, uint32_t request_id) {
	switch(request_id) {
	case 0:
		return trn::ipc::server::RequestHandler<&IHBABIShim::GetProcessHandle>::Handle(this, msg);
	case 1:
		return trn::ipc::server::RequestHandler<&IHBABIShim::GetLoaderConfigEntryCount>::Handle(this, msg);
	case 2:
		return trn::ipc::server::RequestHandler<&IHBABIShim::GetLoaderConfigEntries>::Handle(this, msg);
	case 3:
		return trn::ipc::server::RequestHandler<&IHBABIShim::GetLoaderConfigHandle>::Handle(this, msg);
	case 4:
		return trn::ipc::server::RequestHandler<&IHBABIShim::SetNextLoadPath>::Handle(this, msg);
	case 5:
		return trn::ipc::server::RequestHandler<&IHBABIShim::GetTargetEntryPoint>::Handle(this, msg);
	case 6:
		return trn::ipc::server::RequestHandler<&IHBABIShim::SetExitCode>::Handle(this, msg);
	}
	return 1;
}

trn::ResultCode IHBABIShim::GetProcessHandle(trn::ipc::OutHandle<trn::KProcess, trn::ipc::copy> out) {
	out = *process->proc;
	return RESULT_OK;
}

trn::ResultCode IHBABIShim::GetLoaderConfigEntryCount(trn::ipc::OutRaw<uint32_t> out) {
	out = entries.size();
	return RESULT_OK;
}

trn::ResultCode IHBABIShim::GetLoaderConfigEntries(trn::ipc::Buffer<loader_config_entry_t, 0x6, 0> buffer) {
	size_t count = buffer.size / sizeof(loader_config_entry_t);
	if(count > entries.size()) {
		count = entries.size();
	}
	std::copy_n(entries.begin(), count, buffer.data);
	return RESULT_OK;
}

trn::ResultCode IHBABIShim::GetLoaderConfigHandle(trn::ipc::InRaw<uint32_t> placeholder, trn::ipc::OutHandle<handle_t, trn::ipc::copy> out) {
	if(placeholder.value > handles.size()) {
		return TWILI_ERR_UNRECOGNIZED_HANDLE_PLACEHOLDER;
	}
	out = handles[*placeholder]->handle;
	return RESULT_OK;
}

trn::ResultCode IHBABIShim::SetNextLoadPath(trn::ipc::Buffer<uint8_t, 0x5, 0> path, trn::ipc::Buffer<uint8_t, 0x5, 0> argv) {
	printf("[HBABIShim(0x%x)] next load path: %s[%s]\n", process->proc->handle, path.data, argv.data);
	return RESULT_OK;
}

trn::ResultCode IHBABIShim::GetTargetEntryPoint(trn::ipc::OutRaw<uint64_t> out) {
	out = process->target_entry;
	return RESULT_OK;
}

trn::ResultCode IHBABIShim::SetExitCode(trn::ipc::InRaw<uint32_t> code) {
	printf("[HBABIShim(0x%x)] exit code 0x%x\n", process->proc->handle, code.value);
	return RESULT_OK;
}

}
