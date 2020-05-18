//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2020 misson20000 <xenotoad@xenotoad.net>
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

#pragma once

#include<libtransistor/cpp/ipcclient.hpp>

namespace twili {
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
} // namespace twili
