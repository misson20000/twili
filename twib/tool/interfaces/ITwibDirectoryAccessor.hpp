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

#pragma once

#include<vector>
#include<optional>
#include<tuple>

#include "../RemoteObject.hpp"

namespace twili {
namespace twib {
namespace tool {

class ITwibDirectoryAccessor {
 public:
	ITwibDirectoryAccessor(std::shared_ptr<RemoteObject> obj);

	using CommandID = protocol::ITwibDirectoryAccessor::Command;

	struct DirectoryEntry {
		char path[0x301];
		uint8_t attributes;
		uint32_t entry_type;
		uint64_t file_size;
	};
	
	std::vector<DirectoryEntry> Read();
	uint64_t GetEntryCount();
	
 private:
	std::shared_ptr<RemoteObject> obj;
};

} // namespace tool
} // namespace twib
} // namespace twili
