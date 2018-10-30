//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2018 misson20000 <xenotoad@xenotoad.net>
//
// This file is part of Twili.
//
// Twili is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Twili is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Twili.  If not, see <http://www.gnu.org/licenses/>.
//

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
