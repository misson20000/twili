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

#include "util.hpp"

#include<stdio.h>
#include<errno.h>

namespace twili {
namespace util {

#ifdef WIN32
typedef signed long long ssize_t;
#endif

std::optional<std::vector<uint8_t>> ReadFile(const char *path) {
	FILE *f = fopen(path, "rb");
	if(f == NULL) {
		printf("Failed to open %s\n", path);
		return std::nullopt;
	}

	if(fseek(f, 0, SEEK_END) != 0) {
		printf("Failed to SEEK_END\n");
		fclose(f);
		return std::nullopt;
	}

	ssize_t size = ftell(f);
	if(size == -1) {
		printf("Failed to ftell\n");
		fclose(f);
		return std::nullopt;
	}

	if(fseek(f, 0, SEEK_SET) != 0) {
		printf("Failed to SEEK_SET\n");
		fclose(f);
		return std::nullopt;
	}

	std::vector<uint8_t> buffer(size, 0);

	ssize_t total_read = 0;
	while(total_read < size) {
		size_t r = fread(buffer.data() + total_read, 1, size - total_read, f);
		if(r == 0) {
			printf("Failed to read (read 0x%lx/0x%lx bytes): %d\n", total_read, size, errno);
			fclose(f);
			return std::nullopt;
		}
		total_read+= r;
	}
	fclose(f);

	return buffer;
}

}
}
