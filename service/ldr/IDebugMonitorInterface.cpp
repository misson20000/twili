#include "IDebugMonitorInterface.hpp"

using namespace trn;

namespace twili {
namespace service {
namespace ldr {

IDebugMonitorInterface::IDebugMonitorInterface(ipc_object_t object) : object(object) {
}

IDebugMonitorInterface::IDebugMonitorInterface(ipc::client::Object &&object) : object(std::move(object)) {
}

Result<std::vector<NsoInfo>> IDebugMonitorInterface::GetNsoInfos(uint64_t pid) {
	std::vector<NsoInfo> nso_info;
	uint32_t num_nso_infos = 16;

	Result<std::nullopt_t> r(std::nullopt);
	do {
		nso_info.resize(num_nso_infos);
		
		r = object.SendSyncRequest<2>( // GetNsoInfos
			trn::ipc::InRaw<uint64_t>(pid),
			trn::ipc::OutRaw<uint32_t>(num_nso_infos),
			trn::ipc::Buffer<NsoInfo, 0xA>(nso_info.data(), nso_info.size() * sizeof(NsoInfo)));
	} while(r && num_nso_infos > nso_info.size());
	nso_info.resize(num_nso_infos);
	
	return r.map([&](auto const &_ignored) { return nso_info; });
}

} // namespace ldr
} // namespace service
} // namespace twili
