//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2019 misson20000 <xenotoad@xenotoad.net>
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

#include "ITwibDirectoryAccessor.hpp"

#include<libtransistor/cpp/svc.hpp>

#include "err.hpp"

#include<cstring>

using namespace trn;

namespace twili {
namespace bridge {

ITwibDirectoryAccessor::ITwibDirectoryAccessor(uint32_t object_id, idirectory_t idirectory) : ObjectDispatcherProxy(*this, object_id), idirectory(idirectory), dispatcher(*this) {
	
}

ITwibDirectoryAccessor::~ITwibDirectoryAccessor() {
	ipc_close(idirectory);
}

void ITwibDirectoryAccessor::Read(bridge::ResponseOpener opener) {
	std::vector<idirectoryentry_t> buffer(32);

	uint64_t actual_count;
	ResultCode::AssertOk(idirectory_read(idirectory, &actual_count, buffer.data(), buffer.size() * sizeof(idirectoryentry_t)));
	buffer.resize(actual_count);
	
	opener.RespondOk(std::move(buffer));
}

void ITwibDirectoryAccessor::GetEntryCount(bridge::ResponseOpener opener) {
	uint64_t count;
	ResultCode::AssertOk(idirectory_get_entry_count(idirectory, &count));
	opener.RespondOk(std::move(count));
}

} // namespace bridge
} // namespace twili
