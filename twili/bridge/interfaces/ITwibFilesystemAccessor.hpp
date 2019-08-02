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

#include<libtransistor/ipc/fs/ifilesystem.h>

namespace twili {
namespace bridge {

class ITwibFilesystemAccessor : public ObjectDispatcherProxy<ITwibFilesystemAccessor> {
 public:
	ITwibFilesystemAccessor(uint32_t object_id, ifilesystem_t ifs);
	~ITwibFilesystemAccessor();
	
	using CommandID = protocol::ITwibFilesystemAccessor::Command;
	
 private:
	ifilesystem_t ifs;

	void CreateFile(bridge::ResponseOpener opener, uint32_t mode, uint64_t size, std::string path);
	void DeleteFile(bridge::ResponseOpener opener, std::string path);
	void CreateDirectory(bridge::ResponseOpener opener, std::string path);
	void DeleteDirectory(bridge::ResponseOpener opener, std::string path);
	void DeleteDirectoryRecursively(bridge::ResponseOpener opener, std::string path);
	void RenameFile(bridge::ResponseOpener opener, std::string src, std::string dst);
	void RenameDirectory(bridge::ResponseOpener opener, std::string src, std::string dst);
	void GetEntryType(bridge::ResponseOpener opener, std::string path);
	void OpenFile(bridge::ResponseOpener opener, uint32_t mode, std::string path);
	void OpenDirectory(bridge::ResponseOpener opener, std::string path);

 public:
	SmartRequestDispatcher<
		ITwibFilesystemAccessor,
		SmartCommand<CommandID::CREATE_FILE, &ITwibFilesystemAccessor::CreateFile>,
		SmartCommand<CommandID::DELETE_FILE, &ITwibFilesystemAccessor::DeleteFile>,
		SmartCommand<CommandID::CREATE_DIRECTORY, &ITwibFilesystemAccessor::CreateDirectory>,
		SmartCommand<CommandID::DELETE_DIRECTORY, &ITwibFilesystemAccessor::DeleteDirectory>,
		SmartCommand<CommandID::DELETE_DIRECTORY_RECURSIVELY, &ITwibFilesystemAccessor::DeleteDirectoryRecursively>,
		SmartCommand<CommandID::RENAME_FILE, &ITwibFilesystemAccessor::RenameFile>,
		SmartCommand<CommandID::RENAME_DIRECTORY, &ITwibFilesystemAccessor::RenameDirectory>,
		SmartCommand<CommandID::GET_ENTRY_TYPE, &ITwibFilesystemAccessor::GetEntryType>,
		SmartCommand<CommandID::OPEN_FILE, &ITwibFilesystemAccessor::OpenFile>,
		SmartCommand<CommandID::OPEN_DIRECTORY, &ITwibFilesystemAccessor::OpenDirectory>
	 > dispatcher;
};

} // namespace bridge
} // namespace twili
