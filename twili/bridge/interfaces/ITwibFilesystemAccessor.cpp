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

#include "ITwibFilesystemAccessor.hpp"

#include<libtransistor/cpp/svc.hpp>

#include "err.hpp"

#include "ITwibFileAccessor.hpp"
#include "ITwibDirectoryAccessor.hpp"

#include<cstring>

using namespace trn;

namespace twili {
namespace bridge {

ITwibFilesystemAccessor::ITwibFilesystemAccessor(uint32_t object_id, ifilesystem_t ifs) : ObjectDispatcherProxy(*this, object_id), ifs(ifs), dispatcher(*this) {
	
}

ITwibFilesystemAccessor::~ITwibFilesystemAccessor() {
	ipc_close(ifs);
}

void ITwibFilesystemAccessor::CreateFile(bridge::ResponseOpener opener, uint32_t mode, uint64_t size, std::string path) {
	char path_buffer[0x301];
	std::strncpy(path_buffer, path.c_str(), sizeof(path_buffer));
	ResultCode::AssertOk(ifilesystem_create_file(ifs, mode, size, path_buffer));
	opener.RespondOk();
}

void ITwibFilesystemAccessor::DeleteFile(bridge::ResponseOpener opener, std::string path) {
	char path_buffer[0x301];
	std::strncpy(path_buffer, path.c_str(), sizeof(path_buffer));
	ResultCode::AssertOk(ifilesystem_delete_file(ifs, path_buffer));
	opener.RespondOk();
}

void ITwibFilesystemAccessor::CreateDirectory(bridge::ResponseOpener opener, std::string path) {
	char path_buffer[0x301];
	std::strncpy(path_buffer, path.c_str(), sizeof(path_buffer));
	ResultCode::AssertOk(ifilesystem_create_directory(ifs, path_buffer));
	opener.RespondOk();
}

void ITwibFilesystemAccessor::DeleteDirectory(bridge::ResponseOpener opener, std::string path) {
	char path_buffer[0x301];
	std::strncpy(path_buffer, path.c_str(), sizeof(path_buffer));
	ResultCode::AssertOk(ifilesystem_delete_directory(ifs, path_buffer));
	opener.RespondOk();
}

void ITwibFilesystemAccessor::GetEntryType(bridge::ResponseOpener opener, std::string path) {
	char path_buffer[0x301];
	std::strncpy(path_buffer, path.c_str(), sizeof(path_buffer));

	uint32_t entry_type;
	ResultCode::AssertOk(ifilesystem_get_entry_type(ifs, &entry_type, path_buffer));
	
	opener.RespondOk(std::move(entry_type));
}

void ITwibFilesystemAccessor::OpenFile(bridge::ResponseOpener opener, uint32_t mode, std::string path) {
	char path_buffer[0x301];
	std::strncpy(path_buffer, path.c_str(), sizeof(path_buffer));

	ifile_t ifile;
	ResultCode::AssertOk(ifilesystem_open_file(ifs, &ifile, mode, path_buffer));
	
	opener.RespondOk(opener.MakeObject<ITwibFileAccessor>(ifile));
}

void ITwibFilesystemAccessor::OpenDirectory(bridge::ResponseOpener opener, std::string path) {
	char path_buffer[0x301];
	std::strncpy(path_buffer, path.c_str(), sizeof(path_buffer));

	idirectory_t idir;
	ResultCode::AssertOk(ifilesystem_open_directory(ifs, &idir, 3, path_buffer));
	
	opener.RespondOk(opener.MakeObject<ITwibDirectoryAccessor>(idir));
}

} // namespace bridge
} // namespace twili
