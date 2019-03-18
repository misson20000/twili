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

#include "../Object.hpp"
#include "../ResponseOpener.hpp"
#include "../RequestHandler.hpp"

#include<libtransistor/ipc/fs/idirectory.h>

namespace twili {
namespace bridge {

class ITwibDirectoryAccessor : public ObjectDispatcherProxy<ITwibDirectoryAccessor> {
 public:
	ITwibDirectoryAccessor(uint32_t object_id, idirectory_t id);
	~ITwibDirectoryAccessor();
	
	using CommandID = protocol::ITwibDirectoryAccessor::Command;
	
 private:
	idirectory_t idirectory;

	void Read(bridge::ResponseOpener opener);
	void GetEntryCount(bridge::ResponseOpener opener);

 public:
	SmartRequestDispatcher<
		ITwibDirectoryAccessor,
		SmartCommand<CommandID::READ, &ITwibDirectoryAccessor::Read>,
		SmartCommand<CommandID::GET_ENTRY_COUNT, &ITwibDirectoryAccessor::GetEntryCount>
	 > dispatcher;
};

} // namespace bridge
} // namespace twili
