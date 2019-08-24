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

#include "FileManager.hpp"

#include<libtransistor/cpp/nx.hpp>
#include<libtransistor/ipc/fs/err.h>

#include<sstream>

#include "twili.hpp"

namespace twili {

FileManager::FileManager(Twili &twili) {
	trn_dir_t dir;
	result_t r;

	std::ostringstream path_stream;
	path_stream << "/sd";
	path_stream << twili.config.temp_directory;
	temp_location = path_stream.str();

	std::ostringstream hbabi_path_stream;
	hbabi_path_stream << "sdmc:";
	hbabi_path_stream << twili.config.temp_directory;
	temp_hbabi_location = hbabi_path_stream.str();
	
	printf("opening %s\n", temp_location.c_str());
	r = trn_fs_opendir(&dir, temp_location.c_str());
	if(r != RESULT_OK) {
		if(r == FSPSRV_ERR_NOT_FOUND) {
			printf("  not found, making...\n");
			trn::ResultCode::AssertOk(trn_fs_mkdir(temp_location.c_str()));
			printf("  opening again...\n");
			trn::ResultCode::AssertOk(trn_fs_opendir(&dir, temp_location.c_str()));
		} else {
			printf("  failed: 0x%x\n", r);
			trn::ResultCode::AssertOk(r);
		}
	}

	char path[301];

	printf("opened. clearing...\n");
	
	trn_dirent_t dirent;
	while((r = dir.ops->next(dir.data, &dirent)) == RESULT_OK) {
		printf("found %.*s\n", (int)dirent.name_size, dirent.name);
		snprintf(path, sizeof(path), "%s/%.*s", temp_location.c_str(), (int)dirent.name_size, dirent.name);
		printf("  unlinking %s\n", path);
		trn::ResultCode::AssertOk(trn_fs_unlink(path));
	}
	if(r != LIBTRANSISTOR_ERR_FS_OUT_OF_DIR_ENTRIES) {
		printf("failed to iterate directory\n");
		trn::ResultCode::AssertOk(r);
	}

	printf("prepared directory\n");
}

FILE *FileManager::CreateFile(const char *suffix, std::string &path, std::string &hbabi_path) {
	std::ostringstream path_stream;
	path_stream << temp_location;
	path_stream << "/file_";
	path_stream << next_index;
	path_stream << suffix;

	std::ostringstream hbabi_path_stream;
	hbabi_path_stream << temp_hbabi_location;
	hbabi_path_stream << "/file_";
	hbabi_path_stream << next_index;
	hbabi_path_stream << suffix;

	path = path_stream.str();
	hbabi_path = hbabi_path_stream.str();

	next_index++;
	
	return fopen(path.c_str(), "wb");
}

} // namespace twili
