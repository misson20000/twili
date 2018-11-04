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

#include "NSOTransmutationFile.hpp"

#include<stdio.h>
#include<libtransistor/util.h>

namespace twili {
namespace process {
namespace fs {

static_assert(sizeof(NSOTransmutationFile::Header) == 0x100, "nso header size should be 0x100");

NSOTransmutationFile::NSOTransmutationFile(
	std::unique_ptr<Segment> &&text,
	std::unique_ptr<Segment> &&rodata,
	std::unique_ptr<Segment> &&data,
	size_t bss_size, uint8_t *build_id) :
	TransmutationFile() {
	// generate NSO heade
	size_t file_offset = sizeof(header);
	size_t memory_offset = 0;
	header.text_header.file_offset = file_offset;
	header.text_header.memory_offset = memory_offset;
	header.text_header.decompressed_size = text->Size();
	file_offset+= header.text_header.decompressed_size;
	memory_offset+= header.text_header.decompressed_size;

	header.rodata_header.file_offset = file_offset;
	header.rodata_header.memory_offset = memory_offset;
	header.rodata_header.decompressed_size = rodata->Size();
	file_offset+= header.rodata_header.decompressed_size;
	memory_offset+= header.rodata_header.decompressed_size;

	header.data_header.file_offset = file_offset;
	header.data_header.memory_offset = memory_offset;
	header.data_header.decompressed_size = data->Size();
	file_offset+= header.data_header.decompressed_size;
	memory_offset+= header.data_header.decompressed_size;

	header.bss_size = bss_size;

	if(build_id) {
		memcpy(header.build_id, build_id, 0x20);
	}

	std::unique_ptr<Segment> segments[] = {
		std::make_unique<MemorySegment>((uint8_t*) &header, sizeof(header)),
		std::move(text), std::move(rodata), std::move(data)
	};
	
	SetSegments(std::move(std::vector<std::unique_ptr<Segment>>(std::make_move_iterator(std::begin(segments)), std::make_move_iterator(std::end(segments)))));
}

} // namespace fs
} // namespace process
} // namespace twili
