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

#include "../twili.hpp"

namespace twili {
namespace service {

class IPipe;
class IHBABIShim;
class IAppletShim;

class ITwiliService : public trn::ipc::server::Object {
 public:
	ITwiliService(Twili *twili);
	
	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id);
	trn::ResultCode OpenStdin(trn::ipc::InPid pid, trn::ipc::OutObject<IPipe> &out);
	trn::ResultCode OpenStdout(trn::ipc::InPid pid, trn::ipc::OutObject<IPipe> &out);
	trn::ResultCode OpenStderr(trn::ipc::InPid pid, trn::ipc::OutObject<IPipe> &out);
	trn::ResultCode OpenHBABIShim(trn::ipc::InPid pid, trn::ipc::OutObject<IHBABIShim> &out);
	trn::ResultCode OpenAppletShim(trn::ipc::InPid pid, trn::ipc::InHandle<trn::KProcess, trn::ipc::copy>, trn::ipc::OutObject<IAppletShim> &out);
	trn::ResultCode OpenShellShim(trn::ipc::InPid pid, trn::ipc::InHandle<trn::KProcess, trn::ipc::copy>, trn::ipc::OutObject<IHBABIShim> &out);
	trn::ResultCode CreateNamedOutputPipe(trn::ipc::Buffer<uint8_t, 0x5, 0> name_buffer, trn::ipc::OutObject<IPipe> &val);
	trn::ResultCode Destroy();

  private:
   trn::ResultCode OpenPipe(trn::ipc::InPid pid, trn::ipc::OutObject<IPipe> &out, int fd);
 private:
	Twili *twili;
};

} // namespace service
} // namespace twili
