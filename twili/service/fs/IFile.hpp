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

class IFile : public trn::ipc::server::Object {
 public:
	IFile(trn::ipc::server::IPCServer *server);

	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id) override;
	virtual trn::ResultCode Read(trn::ipc::InRaw<uint32_t> unk0, trn::ipc::InRaw<uint64_t> offset, trn::ipc::InRaw<uint64_t> in_size, trn::ipc::OutRaw<uint64_t> out_size, trn::ipc::Buffer<uint8_t, 0x46, 0> out_buffer) = 0;
	virtual trn::ResultCode GetSize(trn::ipc::OutRaw<uint64_t> file_size) = 0;
};

} // namespace fs
} // namespace service
} // namespace twili
