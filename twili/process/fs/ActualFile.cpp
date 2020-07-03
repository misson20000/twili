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

#include "../../twili.hpp"
#include "err.hpp"

namespace twili {
namespace process {
namespace fs {

ActualFile::ActualFile(FILE *file) : file(file) {
	if(file == NULL) {
		// don't construct with null files, please. use Open instead.
		twili::Abort(TWILI_ERR_IO_ERROR);
	}
}

trn::ResultCode ActualFile::Open(const char *path, std::shared_ptr<ActualFile> *out) {
	FILE *file = fopen(path, "rb");
	
	if(file == NULL) {
		return TWILI_ERR_IO_ERROR;
	}

	*out = std::make_shared<ActualFile>(file);
	
	return RESULT_OK;
}

ActualFile::~ActualFile() {
	fclose(file);
}

trn::ResultCode ActualFile::Read(size_t offset, size_t size, uint8_t *out, size_t *out_size) {
	if(fseek(file, offset, SEEK_SET) != 0) {
		return TWILI_ERR_IO_ERROR;
	}
	*out_size = fread((void*) out, 1, size, file);
	return RESULT_OK;
}

trn::ResultCode ActualFile::GetSize(size_t *out_size) {
	if(!has_size) {
		if(fseek(file, 0, SEEK_END) != 0) {
			return TWILI_ERR_IO_ERROR;
		}
		off_t out = ftello(file);
		if(out == -1) {
			return TWILI_ERR_IO_ERROR;
		}
		size = out;
		has_size = true;
	}
	*out_size = size;
	return RESULT_OK;
}

} // namespace fs
} // namespace process
} // namespace twili
