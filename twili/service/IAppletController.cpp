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

#include "IAppletController.hpp"

#include "../twili.hpp"

#include "err.hpp"

namespace twili {
namespace service {

IAppletController::IAppletController(trn::ipc::server::IPCServer *server, std::shared_ptr<process::AppletProcess> process) : trn::ipc::server::Object(server), process(process) {
}

IAppletController::~IAppletController() {
	printf("destroying IAppletController\n");
	printf("marking process as exited\n");
	process->ChangeState(process::MonitoredProcess::State::Exited);
}

trn::ResultCode IAppletController::Dispatch(trn::ipc::Message msg, uint32_t request_id) {
	switch(request_id) {
	case 0:
		return trn::ipc::server::RequestHandler<&IAppletController::SetResult>::Handle(this, msg);
	case 1:
		return trn::ipc::server::RequestHandler<&IAppletController::GetEvent>::Handle(this, msg);
	case 2:
		return trn::ipc::server::RequestHandler<&IAppletController::GetCommand>::Handle(this, msg);
		
	}
	return 1;
}

trn::ResultCode IAppletController::SetResult(trn::ipc::InRaw<uint32_t> result) {
	process->SetResult(result.value);
	return RESULT_OK;
}

trn::ResultCode IAppletController::GetEvent(trn::ipc::OutHandle<handle_t, trn::ipc::copy> event) {
	event = process->GetCommandEvent().handle;
	return RESULT_OK;
}

trn::ResultCode IAppletController::GetCommand(trn::ipc::OutRaw<uint32_t> command) {
	command = process->PopCommand();
	return RESULT_OK;
}

} // namespace service
} // namespace twili
