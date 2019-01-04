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

namespace twili {
namespace service {
namespace fs {

class IFile;

class IFileSystem : public trn::ipc::server::Object {
 public:
	IFileSystem(trn::ipc::server::IPCServer *server);

	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id) override;
	virtual trn::ResultCode OpenFile(trn::ipc::InRaw<uint32_t> mode, trn::ipc::Buffer<char, 0x19, 0> path, trn::ipc::OutObject<IFile> &out) = 0;
	virtual trn::ResultCode GetFileTimeStampRaw(trn::ipc::Buffer<char, 0x19, 0> path, trn::ipc::OutRaw<char[0x20]> timestamp);
};

} // namespace fs
} // namespace service
} // namespace twili
