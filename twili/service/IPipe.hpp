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

#include "../TwibPipe.hpp"

namespace twili {
namespace service {

class IPipe : public trn::ipc::server::Object {
 public:
	IPipe(trn::ipc::server::IPCServer *server);
	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id) = 0;
};

class IPipeStandard : public IPipe {
 public:
	IPipeStandard(trn::ipc::server::IPCServer *server, int fd);
	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id);

	trn::ResultCode Read(trn::ipc::OutRaw<uint64_t> size, trn::ipc::Buffer<uint8_t, 0x6, 0> buffer);
	trn::ResultCode Write(trn::ipc::Buffer<uint8_t, 0x5, 0> buffer);
 private:
	int fd;
};

class IPipeTwib : public IPipe {
 public:
	IPipeTwib(trn::ipc::server::IPCServer *server, std::shared_ptr<TwibPipe> pipe);
	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id);

	trn::ResultCode Read(std::function<void(trn::ResultCode)> cb, trn::ipc::OutRaw<uint64_t> size, trn::ipc::Buffer<uint8_t, 0x6, 0> buffer);
	trn::ResultCode Write(std::function<void(trn::ResultCode)> cb, trn::ipc::Buffer<uint8_t, 0x5, 0> buffer);
 private:
	std::shared_ptr<TwibPipe> pipe;
};

} // namespace service
} // namespace twili
