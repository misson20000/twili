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

#include "PFS0BuilderFile.hpp"

namespace twili {
namespace process {
namespace fs {

struct PFS0Header {
	uint32_t magic = 0x30534650;
	uint32_t num_files;
	uint32_t string_table_size;
	uint32_t padding;
};

struct PFS0Entry {
	uint64_t file_image_offset;
	uint64_t size;
	uint32_t string_table_offset;
};

static_assert(sizeof(PFS0Header) == 0x10, "pfs0 header size should be 0x10");
static_assert(sizeof(PFS0Entry) == 0x18, "pfs0 entry size should be 0x18");

PFS0BuilderFile::PFS0BuilderFile() {
}

void PFS0BuilderFile::Append(std::string name, std::shared_ptr<ProcessFile> file) {
	Entry e;
	e.name = name;
	e.file = file;
	e.string_table_offset = string_table_size;
	e.file_image_offset = file_image_size;

	string_table_size+= name.size() + 1;
	file_image_size+= file->GetSize();
	
	entries.push_back(e);
}

size_t PFS0BuilderFile::Read(size_t offset, size_t size, uint8_t *out) {
	size_t total_read = 0;

	printf("pfs0: read offset=0x%lx size=0x%lx\n", offset, size);
	
	// Header
	if(offset < sizeof(PFS0Header) && size > 0) {
		PFS0Header header;
		header.num_files = entries.size();
		header.string_table_size = string_table_size;
		size_t sz = std::min(size, sizeof(PFS0Header) - offset);
		std::copy_n((uint8_t*) &header + offset, sz, out);
		out+= sz;
		total_read+= sz;
		offset+= sz;
		size-= sz;
	}
	offset-= sizeof(PFS0Header);
	printf("pfs0: fet(offset=0x%lx size=0x%lx)\n", offset, size);

	// File Entry Table
	if(offset < sizeof(PFS0Entry)*entries.size() && size > 0) {
		auto i = entries.begin() + (offset/sizeof(PFS0Entry));
		while(size > 0 && i != entries.end()) {
			size_t local_offset = offset % sizeof(PFS0Entry);
			size_t sz = std::min(size, sizeof(PFS0Entry) - local_offset);
			
			PFS0Entry e;
			e.file_image_offset = i->file_image_offset;
			e.string_table_offset = i->string_table_offset;
			e.size = i->file->GetSize();
			std::copy_n((uint8_t*) &e + local_offset, sz, out);
			out+= sz;
			total_read+= sz;
			offset+= sz;
			size-= sz;
			i++;
		}
	}
	offset-= sizeof(PFS0Entry)*entries.size();

	// Padding
	printf("pfs0: pad(offset=0x%lx size=0x%lx)\n", offset, size);
	size_t fet_end = sizeof(PFS0Header) + sizeof(PFS0Entry) * entries.size();
	size_t strtab_offset = (fet_end+0x1f) & ~0x1f;
	size_t strtab_pad = strtab_offset - fet_end;
	if(offset < strtab_pad) {
		out+= strtab_pad;
		total_read+= strtab_pad;
		size-= strtab_pad;
	}

	// String Table
	printf("pfs0: strtab(offset=0x%lx size=0x%lx)\n", offset, size);
	if(offset < string_table_size && size > 0) {
		auto i = std::lower_bound(
			entries.begin(), entries.end(), offset,
			[](Entry &e, size_t offset) {
				return (e.string_table_offset + e.name.size() + 1) < offset;
			});
		while(offset < string_table_size && size > 0 && i != entries.end()) {
			size_t local_offset = (offset - i->string_table_offset);
			size_t sz = std::min(size, i->name.size() - local_offset);
			printf("  reading 0x%lx bytes from +0x%lx in \"%s\"\n", sz, local_offset, i->name.c_str());
			std::copy_n(i->name.begin() + local_offset, sz, out);
			if(size > i->name.size() - local_offset) {
				out[sz++] = 0; // null terminator
			}
			out+= sz;
			total_read+= sz;
			offset+= sz;
			size-= sz;
			printf("  offset is 0x%lx\n", offset);
			while(i != entries.end() && i->string_table_offset + i->name.size() + 1 <= offset) {
				printf("  advancing past \"%s\"\n", i->name.c_str());
				i++;
			}
		}
	}
	offset-= string_table_size;

	// File Images
	printf("pfs0: img(offset=0x%lx size=0x%lx)\n", offset, size);
	if(offset < file_image_size && size > 0) {
		auto i = std::lower_bound(
			entries.begin(), entries.end(), offset,
			[](Entry &e, size_t offset) {
				return (e.file_image_offset + e.file->GetSize()) < offset;
			});
		while(offset < file_image_size && size > 0 && i != entries.end()) {
			size_t local_offset = (offset - i->file_image_offset);
			size_t sz = std::min(size, i->file->GetSize() - local_offset);
			printf("  reading 0x%lx bytes from +0x%lx in %s\n", sz, local_offset, i->name.c_str());
			i->file->Read(local_offset, sz, out);
			out+= sz;
			total_read+= sz;
			offset+= sz;
			size-= sz;
			while(i != entries.end() && i->file_image_offset + i->file->GetSize() <= offset) {
				i++;
			}
		}
	}

	return total_read;
}

size_t PFS0BuilderFile::GetSize() {
	size_t total_size = 0;
	total_size+= sizeof(PFS0Header);
	total_size+= sizeof(PFS0Entry) * entries.size();
	total_size = (total_size + 0x1f) & ~0x1f;
	total_size+= string_table_size;
	total_size+= file_image_size;
	return total_size;
}

} // namespace fs
} // namespace process
} // namespace twili
