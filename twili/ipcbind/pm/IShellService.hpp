#pragma once

#include<libtransistor/cpp/ipcclient.hpp>

namespace twili {
namespace service {
namespace pm {

class IShellService {
 public:
	IShellService() = default;
	IShellService(ipc_object_t object);
	IShellService(trn::ipc::client::Object &&object);

	trn::Result<std::nullopt_t> TerminateProcessByPid(uint64_t pid);
	
	trn::ipc::client::Object object;
};

} // namespace pm
} // namespace service
} // namespace twili
