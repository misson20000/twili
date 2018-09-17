#include "IGeneralService.hpp"

using namespace trn;

namespace twili {
namespace service {
namespace nifm {

IGeneralService::IGeneralService(ipc_object_t object) : object(object) {
}

IGeneralService::IGeneralService(ipc::client::Object &&object) : object(std::move(object)) {
}

IRequest IGeneralService::CreateRequest(uint32_t requirement_preset) {
	IRequest rq;
	ResultCode::AssertOk(
		object.SendSyncRequest<4>(
			ipc::InRaw<uint32_t>(requirement_preset),
			ipc::OutObject(rq)));
	return rq;
}

} // namespace nifm
} // namespace service
} // namespace twili
