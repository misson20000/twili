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

#include "VectorFile.hpp"

namespace twili {
namespace process {
namespace fs {

VectorFile::VectorFile(std::vector<uint8_t> vector) : vector(vector) {
}

trn::ResultCode VectorFile::Read(size_t offset, size_t size, uint8_t *out, size_t *size_out) {
	size_t actual_size = std::min(vector.size() - offset, size);
	std::copy_n(vector.begin() + offset, actual_size, out);
	*size_out = actual_size;
	
	return RESULT_OK;
}

trn::ResultCode VectorFile::GetSize(size_t *size_out) {
	*size_out = vector.size();
	
	return RESULT_OK;
}

} // namespace fs
} // namespace process
} // namespace twili
