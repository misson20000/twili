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

#include "ActualFile.hpp"

#include<libtransistor/cpp/nx.hpp>

#include "err.hpp"

namespace twili {
namespace process {
namespace fs {

ActualFile::ActualFile(FILE *file) : file(file) {
	if(file == NULL) {
		printf("failed to open file\n");
		throw trn::ResultError(TWILI_ERR_IO_ERROR);
	}
}

ActualFile::ActualFile(const char *path) : file(fopen(path, "rb")) {
	if(file == NULL) {
		printf("failed to open '%s'\n", path);
		throw trn::ResultError(TWILI_ERR_IO_ERROR);
	}
}

ActualFile::~ActualFile() {
	if(file != NULL) {
		fclose(file);
	}
}

size_t ActualFile::Read(size_t offset, size_t size, uint8_t *out) {
	if(fseek(file, offset, SEEK_SET) != 0) {
		throw trn::ResultError(TWILI_ERR_IO_ERROR);
	}
	return fread((void*) out, 1, size, file);
}

size_t ActualFile::GetSize() {
	if(fseek(file, 0, SEEK_END) != 0) {
		throw trn::ResultError(TWILI_ERR_IO_ERROR);
	}
	off_t out = ftello(file);
	if(out == -1) {
		throw trn::ResultError(TWILI_ERR_IO_ERROR);
	}
	return out;
}

} // namespace fs
} // namespace process
} // namespace twili
