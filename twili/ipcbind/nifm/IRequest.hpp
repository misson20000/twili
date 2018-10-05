#pragma once

#include<libtransistor/cpp/ipcclient.hpp>

namespace twili {
namespace service {
namespace nifm {

class IRequest {
 public:
	enum class State : uint32_t {
		Error = 1,
		Pending = 2,
		Connected = 3,
	};
	IRequest() = default;
	IRequest(ipc_object_t object);
	IRequest(trn::ipc::client::Object &&object);

	State GetRequestState();
	trn::ResultCode GetResult();
	std::tuple<trn::KEvent, trn::KEvent> GetSystemEventReadableHandles();
	void Cancel();
	void Submit();
	void SetConnectionConfirmationOption(uint8_t option);
	void SetPersistent(bool);
	
	trn::ipc::client::Object object;
};

} // namespace nifm
} // namespace service
} // namespace twili
