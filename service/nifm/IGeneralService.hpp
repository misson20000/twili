#pragma once

#include<libtransistor/cpp/ipcclient.hpp>

#include "IRequest.hpp"

namespace twili {
namespace service {
namespace nifm {

class IGeneralService {
 public:
	IGeneralService() = default;
	IGeneralService(ipc_object_t object);
	IGeneralService(trn::ipc::client::Object &&object);

	IRequest CreateRequest(uint32_t requirement_preset);

	trn::ipc::client::Object object;
};

} // namespace nifm
} // namespace service
} // namespace twili
