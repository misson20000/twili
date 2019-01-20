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

#include "IShellService.hpp"

using namespace trn;

namespace twili {
namespace service {
namespace pm {

IShellService::IShellService(ipc_object_t object) : object(object) {
}

IShellService::IShellService(ipc::client::Object &&object) : object(std::move(object)) {
}

trn::Result<std::nullopt_t> IShellService::TerminateProcessByPid(uint64_t pid) {
	return object.SendSyncRequest<1>(
		ipc::InRaw<uint64_t>(pid));
}

trn::Result<uint64_t> IShellService::LaunchProcess(uint32_t flags, uint64_t tid, uint64_t storage) {
	uint64_t pid;
	return object.SendSyncRequest<0>(
		ipc::InRaw<uint32_t>(flags),
		ipc::InRaw<uint64_t>(tid),
		ipc::InRaw<uint64_t>(storage),
		ipc::OutRaw<uint64_t>(pid))
		.map([&](auto &&_) { return pid; });
}

} // namespace pm
} // namespace service
} // namespace twili
