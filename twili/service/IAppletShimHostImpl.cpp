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

#include "IAppletShim.hpp"

#include "IHBABIShim.hpp"

namespace twili {
namespace service {

IAppletShim::HostImpl::HostImpl(trn::ipc::server::IPCServer *server, std::shared_ptr<process::AppletProcess> process) : IAppletShim(server), process(process) {
}

trn::ResultCode IAppletShim::HostImpl::GetMode(trn::ipc::OutRaw<applet_shim::Mode> mode) {
	mode = applet_shim::Mode::Host;
	return RESULT_OK;
}

trn::ResultCode IAppletShim::HostImpl::OpenHBABIShim(trn::ipc::OutObject<IHBABIShim> &hbabishim) {
	auto r = server->CreateObject<IHBABIShim>(this, process);
	if(r) {
		hbabishim.value = r.value();
		return RESULT_OK;
	} else {
		return r.error().code;
	}
}

} // namespace service
} // namespace twili
