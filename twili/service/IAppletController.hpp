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

#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>

#include "../process/AppletProcess.hpp"

namespace twili {
namespace service {

class IAppletController : public trn::ipc::server::Object {
 public:
	IAppletController(trn::ipc::server::IPCServer *server, std::shared_ptr<process::AppletProcess> process);
	virtual ~IAppletController() override;
	
	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id) override;

	trn::ResultCode SetResult(trn::ipc::InRaw<uint32_t> result);
	trn::ResultCode GetEvent(trn::ipc::OutHandle<handle_t, trn::ipc::copy> event);
	trn::ResultCode GetCommand(trn::ipc::OutRaw<uint32_t> command);
 private:
	std::shared_ptr<process::AppletProcess> process;
};

} // namespace service
} // namespace twili
