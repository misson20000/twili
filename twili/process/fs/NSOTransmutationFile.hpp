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

#pragma once

#include<stdlib.h>
#include<stdint.h>

#include "TransmutationFile.hpp"

namespace twili {
namespace process {
namespace fs {

class NSOTransmutationFile : public TransmutationFile {
 public:
	NSOTransmutationFile(
		std::unique_ptr<Segment> &&text,
		std::unique_ptr<Segment> &&rodata,
		std::unique_ptr<Segment> &&data,
		size_t bss_size, uint8_t *build_id);
	struct SegmentHeader {
		uint32_t file_offset = 0;
		uint32_t memory_offset = 0;
		uint32_t decompressed_size = 0;
	};
	struct Header {
		uint32_t magic = 0x304f534e; // NSO0
		uint32_t version = 0;
		uint32_t reserved = 0;
		uint32_t flags = 0; // no compression or sum check
		SegmentHeader text_header;
		uint32_t module_offset = 0;
		SegmentHeader rodata_header;
		uint32_t module_file_size = 0;
		SegmentHeader data_header;
		uint32_t bss_size = 0;
		uint8_t build_id[0x20] = {0};
		uint32_t text_compressed = 0;
		uint32_t rodata_compressed = 0;
		uint32_t data_compressed = 0;
		uint8_t reserved2[0x1c] = {0};
		uint64_t api_info_extents = 0;
		uint64_t dyn_str_extents = 0;
		uint64_t dyn_sym_extents = 0;
		uint8_t hashes[0x60] = {0};
	} header;
};

} // namespace fs
} // namespace process
} // namespace twili
