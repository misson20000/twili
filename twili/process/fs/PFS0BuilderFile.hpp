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

#include<stdlib.h>
#include<stdint.h>

#include "ProcessFile.hpp"

#include<string>
#include<vector>

namespace twili {
namespace process {
namespace fs {

class PFS0BuilderFile : public ProcessFile {
 public:
	PFS0BuilderFile();

	void Append(std::string name, std::shared_ptr<ProcessFile> file);
	virtual size_t Read(size_t offset, size_t size, uint8_t *out) override;
	virtual size_t GetSize() override;
 private:
	size_t string_table_size = 0;
	size_t file_image_size = 0;
	
	struct Entry {
		std::string name;
		std::shared_ptr<ProcessFile> file;

		size_t string_table_offset;
		size_t file_image_offset;
	};
	
	std::vector<Entry> entries;
};

} // namespace fs
} // namespace process
} // namespace twili
