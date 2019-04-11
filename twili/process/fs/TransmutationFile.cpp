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

#include "TransmutationFile.hpp"

#include<stdio.h>

namespace twili {
namespace process {
namespace fs {

TransmutationFile::TransmutationFile() {
}

void TransmutationFile::SetSegments(std::vector<std::unique_ptr<Segment>> &&segments) {
	this->segments = std::move(segments);
}

size_t TransmutationFile::Read(size_t offset, size_t size, uint8_t *out) {
	size_t segment_begin = 0;
	size_t segment_end = 0;
	size_t total_read = 0;
	
	// read from virtual segments
	for(auto i = segments.begin(); i != segments.end() && size > 0; i++) {
		segment_end = segment_begin + (*i)->Size();
		if(offset < segment_end) {
			size_t seg_off = offset - segment_begin;
			size_t seg_size = segment_end - offset;
			if(seg_size > size) {
				seg_size = size;
			}
			size_t actual_read = (*i)->Read(seg_off, seg_size, out);
			if(actual_read < seg_size) {
				// if we get a short read, give up
				printf(
					"TransmutationFile: short read [(0x%lx, 0x%lx) -> (0x%lx, 0x%lx)]\n",
					offset, seg_size,
					seg_off, actual_read);
				return total_read + actual_read;
			}
			offset+= seg_size;
			out+= seg_size;
			total_read+= seg_size;
			size-= seg_size;
		}
		segment_begin = segment_end;
	}
	return total_read;
}

size_t TransmutationFile::GetSize() {
	size_t total_size = 0;
	for(auto i = segments.begin(); i != segments.end(); i++) {
		total_size+= (*i)->Size();
	}
	return total_size;
}

TransmutationFile::BackedSegment::BackedSegment(std::shared_ptr<ProcessFile> file, size_t file_offset, size_t size) :
	file(file), file_offset(file_offset), size(size) {
}

size_t TransmutationFile::BackedSegment::Read(size_t offset, size_t size, uint8_t *out) {
	return file->Read(file_offset + offset, size, out);
}

size_t TransmutationFile::BackedSegment::Size() {
	return size;
}

TransmutationFile::MemorySegment::MemorySegment(uint8_t *buffer, size_t size) : buffer(buffer), size(size) {
}

size_t TransmutationFile::MemorySegment::Read(size_t offset, size_t size, uint8_t *out) {
	std::copy_n(buffer + offset, size, out);
	return size;
}

size_t TransmutationFile::MemorySegment::Size() {
	return size;
}

} // namespace fs
} // namespace process
} // namespace twili
