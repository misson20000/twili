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

#include<libtransistor/ipc/fs/ifile.h>

namespace twili {
namespace bridge {

class ITwibFileAccessor : public ObjectDispatcherProxy<ITwibFileAccessor> {
 public:
	ITwibFileAccessor(uint32_t object_id, ifile_t ifile);
	~ITwibFileAccessor();
	
	using CommandID = protocol::ITwibFileAccessor::Command;
	
 private:
	ifile_t ifile;

	void Read(bridge::ResponseOpener opener, uint64_t offset, uint64_t size);
	void Write(bridge::ResponseOpener opener, uint64_t offset, InputStream &stream);
	void Flush(bridge::ResponseOpener opener);
	void SetSize(bridge::ResponseOpener opener, uint64_t size);
	void GetSize(bridge::ResponseOpener opener);

 public:
	SmartRequestDispatcher<
		ITwibFileAccessor,
		SmartCommand<CommandID::READ, &ITwibFileAccessor::Read>,
		SmartCommand<CommandID::WRITE, &ITwibFileAccessor::Write>,
		SmartCommand<CommandID::FLUSH, &ITwibFileAccessor::Flush>,
		SmartCommand<CommandID::SET_SIZE, &ITwibFileAccessor::SetSize>,
		SmartCommand<CommandID::GET_SIZE, &ITwibFileAccessor::GetSize>
	 > dispatcher;
};

} // namespace bridge
} // namespace twili
