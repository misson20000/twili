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

#include "Protocol.hpp"

#include<cstring>

namespace twili {
namespace twib {
namespace tool {

ITwibDirectoryAccessor::ITwibDirectoryAccessor(std::shared_ptr<RemoteObject> obj) : obj(obj) {
	
}

std::vector<ITwibDirectoryAccessor::DirectoryEntry> ITwibDirectoryAccessor::Read() {
	std::vector<DirectoryEntry> vec;
	obj->SendSmartSyncRequest(
		CommandID::READ,
		out<std::vector<DirectoryEntry>>(vec));
	return vec;
}

uint64_t ITwibDirectoryAccessor::GetEntryCount() {
	uint64_t count;
	obj->SendSmartSyncRequest(CommandID::GET_ENTRY_COUNT, out<uint64_t>(count));
	return count;
}


} // namespace tool
} // namespace twib
} // namespace twili
