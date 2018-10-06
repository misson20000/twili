#include "AppletProcess.hpp"

#include "../twili.hpp"

#include "err.hpp"

using namespace trn;

namespace twili {
namespace process {

AppletProcess::AppletProcess(Twili &twili, std::vector<uint8_t> nro) :
	MonitoredProcess(twili),
	reader(nro) {
	builder.AppendNRO(reader);
}

void AppletProcess::Launch() {
	twili.applet_tracker.QueueLaunch(shared_from_this());
}

void AppletProcess::AddHBABIEntries(std::vector<loader_config_entry_t> &entries) {
	MonitoredProcess::AddHBABIEntries(entries);
	entries.push_back({
			.key = LCONFIG_KEY_APPLET_TYPE,
			.flags = 0,
			.applet_type = {
				.applet_type = LCONFIG_APPLET_TYPE_LIBRARY_APPLET,
			}
		});
}

void AppletProcess::Attach(trn::KProcess &&process) {
	proc = std::make_shared<trn::KProcess>(std::move(process));
}

size_t AppletProcess::GetTargetSize() {
	return builder.GetTotalSize();
}

void AppletProcess::SetupTarget() {
	// find the extra memory the loader gave us

	uint64_t load_addr = 0;
	uint64_t extramem_size = 0;
	
	uint64_t addr = 0;
	memory_info_t meminfo;
	printf("setting up target...\n");
	do {
		meminfo = std::get<0>(ResultCode::AssertOk(svc::QueryProcessMemory(*proc, addr)));
		if(meminfo.memory_type == 3) {
			printf("  found CodeStatic at 0x%lx, perm %d\n", (uint64_t) meminfo.base_addr, meminfo.permission);
			if(meminfo.permission == 0) {
				printf("    this is likely our extra memory\n");
				load_addr = (uint64_t) meminfo.base_addr;
				extramem_size = meminfo.size;
			}
		}
		
		addr = (uint64_t) meminfo.base_addr + meminfo.size;
	} while(addr > 0);

	if(load_addr == 0) {
		printf("couldn't find extra memory\n");
		throw ResultError(TWILI_ERR_EXTRA_MEMORY_NOT_FOUND);
	}

	if(extramem_size < GetTargetSize()) {
		printf("not enough extra memory. expected 0x%lx bytes, got 0x%lx bytes\n",
					 GetTargetSize(), extramem_size);
		throw ResultError(TWILI_ERR_EXTRA_MEMORY_NOT_FOUND);
	}

	target_entry = load_addr;
	
	ResultCode::AssertOk(builder.Load(proc, load_addr));
}

} // namespace process
} // namespace twili
