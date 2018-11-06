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

#include "IFileSystem.hpp"

namespace twili {
namespace service {
namespace fs {

IFileSystem::IFileSystem(trn::ipc::server::IPCServer *server) : trn::ipc::server::Object(server) {
}

trn::ResultCode IFileSystem::Dispatch(trn::ipc::Message msg, uint32_t request_id) {
	switch(request_id) {
	case 8:
		return trn::ipc::server::RequestHandler<&IFileSystem::OpenFile>::Handle(this, msg);
	}
	return 1;
}

} // namespace fs
} // namespace service
} // namespace twili
