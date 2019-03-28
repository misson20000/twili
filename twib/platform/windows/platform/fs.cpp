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

#include "platform.hpp"

namespace twili {
namespace platform {
namespace fs {

std::optional<Stat> StatFile(const char *path) {
	DWORD attributes = GetFileAttributes(path);
	if(attributes == INVALID_FILE_ATTRIBUTES) {
		DWORD err = GetLastError();
		if(err != ERROR_PATH_NOT_FOUND && err != ERROR_FILE_NOT_FOUND) {
			throw NetworkError(err);
		}
		return std::nullopt;
	} else {
		Stat out;
		out.is_directory = attributes & FILE_ATTRIBUTE_DIRECTORY;
	}
}

std::string BaseName(const char *path) {
	char name_buffer[512];
	char ext_buffer[64];

	std::string out;

	_splitpath_s(path, nullptr, 0, nullptr, 0, name_buffer, sizeof(name_buffer), ext_buffer, sizeof(ext_buffer));

	out+= name_buffer;
	out+= '.';
	out+= ext_buffer;

	return out;
}

} // namespace fs
} // namespace platform
} // namespace twili