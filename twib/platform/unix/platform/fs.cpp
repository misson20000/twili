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

#include<vector>

#include<libgen.h>
#include<fcntl.h>
#include<sys/stat.h>

namespace twili {
namespace platform {
namespace fs {

std::optional<Stat> StatFile(const char *path) {
	struct stat stat_buf;
	if(stat(path, &stat_buf) == -1) {
		if(errno != ENOENT) {
			throw NetworkError(errno);
		}
		return std::nullopt;
	} else {
		Stat out;
		out.is_directory = S_ISDIR(stat_buf.st_mode);
		return out;
	}
}

std::string BaseName(const char *path) {
	std::string copy(path);
	return basename(copy.data()); // haha don't do this
}

} // namespace fs
} // namespace platform
} // namespace twili