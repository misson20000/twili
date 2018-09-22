#include "IRequest.hpp"

using namespace trn;

namespace twili {
namespace service {
namespace nifm {

IRequest::IRequest(ipc_object_t object) : object(object) {
}

IRequest::IRequest(ipc::client::Object &&object) : object(std::move(object)) {
}

IRequest::State IRequest::GetRequestState() {
	State state;
	ResultCode::AssertOk(
		object.SendSyncRequest<0>(
			ipc::OutRaw(state)));
	return state;
}

ResultCode IRequest::GetResult() {
	auto r = object.SendSyncRequest<1>();
	if(r) {
		return 0;
	} else {
		return r.error();
	}
}

std::tuple<trn::KEvent, trn::KEvent> IRequest::GetSystemEventReadableHandles() {
	trn::KEvent h1, h2;
	ResultCode::AssertOk(
		object.SendSyncRequest<2>(
			ipc::OutHandle<trn::KEvent, ipc::copy>(h1),
			ipc::OutHandle<trn::KEvent, ipc::copy>(h2)));
	return std::make_tuple(std::move(h1), std::move(h2));
}

void IRequest::Cancel() {
	ResultCode::AssertOk(
		object.SendSyncRequest<3>());
}

void IRequest::Submit() {
	ResultCode::AssertOk(
		object.SendSyncRequest<4>());
}

void IRequest::SetConnectionConfirmationOption(uint8_t option) {
	ResultCode::AssertOk(
		object.SendSyncRequest<11>(
			ipc::InRaw(option)));
}

void IRequest::SetPersistent(bool option) {
	ResultCode::AssertOk(
		object.SendSyncRequest<12>(
			ipc::InRaw(option)));
}

} // namespace nifm
} // namespace service
} // namespace twili
