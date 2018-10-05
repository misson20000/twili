#pragma once

#include<libtransistor/cpp/ipcclient.hpp>

namespace twili {
namespace service {
namespace ldr {

struct NsoInfo {
	uint64_t addr;
	size_t size;
	union {
		uint8_t build_id[0x20];
		uint64_t build_id_64[4];
	};
};

class IDebugMonitorInterface {
 public:
	IDebugMonitorInterface() = default;
	IDebugMonitorInterface(ipc_object_t object);
	IDebugMonitorInterface(trn::ipc::client::Object &&object);

	trn::Result<std::vector<NsoInfo>> GetNsoInfos(uint64_t pid);
	
	trn::ipc::client::Object object;
};

} // namespace ldr
} // namespace service
} // namespace twili
