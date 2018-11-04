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

#include<libtransistor/cpp/nx.hpp>

#include<stdio.h>
#include<unistd.h>

#include "applet_shim.hpp"
#include "err.hpp"

using namespace trn;

namespace twili {
namespace applet_shim {

void ControlMode(ipc::client::Object &iappletshim);
void HostMode(ipc::client::Object &iappletshim);

} // namespace applet_shim
} // namespace twili

int main(int argc, char *argv[]) {
	try {
		twili_pipe_t twili_out;
		trn::ResultCode::AssertOk(twili_init());
		trn::ResultCode::AssertOk(twili_open_stdout(&twili_out));
		int fd = twili_pipe_fd(&twili_out);
		dup2(fd, STDOUT_FILENO); // stdout is available for debugging
		close(fd);
		printf("applet_shim says hello\n");

		trn::ResultCode::AssertOk(fatal_init());
		
		ipc::client::Object iappletshim;
		{
			service::SM sm = ResultCode::AssertOk(service::SM::Initialize());

			// connect to twili
			ipc::client::Object itwiliservice = 
				ResultCode::AssertOk(sm.GetService("twili"));

			ResultCode::AssertOk(
				itwiliservice.SendSyncRequest<4>( // OpenAppletShim
					ipc::InPid(),
					ipc::InHandle<handle_t, ipc::copy>(0xffff8001),
					ipc::OutObject(iappletshim)));
		}

		printf("got IAppletShim\n");
		
		twili::applet_shim::Mode mode;
		ResultCode::AssertOk(
			iappletshim.SendSyncRequest<0>( // GetMode
				ipc::OutRaw(mode)));

		switch(mode) {
		case twili::applet_shim::Mode::Control:
			twili::applet_shim::ControlMode(iappletshim);
		case twili::applet_shim::Mode::Host:
			twili::applet_shim::HostMode(iappletshim);
			break;
		default:
			printf("fatal: unknown mode %d\n", (uint32_t) mode);
			fatal_transition_to_fatal_error(TWILI_ERR_APPLET_SHIM_UNKNOWN_MODE, 0);
			break;
		}
		
		return 0;
	} catch(trn::ResultError &e) {
		printf("applet_shim: caught 0x%x\n", e.code.code);
		return e.code.code;
	}

	return 0;
}
