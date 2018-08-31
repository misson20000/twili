#include "IShellService.hpp"

using namespace trn;

namespace twili {
namespace service {
namespace pm {

IShellService::IShellService(ipc_object_t object) : object(object) {
}

IShellService::IShellService(ipc::client::Object object) : object(std::move(object)) {
}

trn::Result<std::nullopt_t> IShellService::TerminateProcessByPid(uint64_t pid) {
	return object.SendSyncRequest<1>(
		ipc::InRaw<uint64_t>(pid));
}

} // namespace pm
} // namespace service
} // namespace twili
