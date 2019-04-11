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

#include<vector>

#include "ProcessFile.hpp"

namespace twili {
namespace process {
namespace fs {

class TransmutationFile : public ProcessFile {
 public:
	class Segment {
	 public:
		virtual ~Segment() = default;
		// It's up to TransmutationFile to try to avoid out-of-bounds reads,
		// but give a way to report them if they do happen for some reason.
		virtual size_t Read(size_t offset, size_t size, uint8_t *out) = 0;
		virtual size_t Size() = 0;
	};

	TransmutationFile();
	virtual size_t Read(size_t offset, size_t size, uint8_t *out);
	virtual size_t GetSize();
	
	class BackedSegment : public Segment {
	 public:
		BackedSegment(std::shared_ptr<ProcessFile> file, size_t file_offset, size_t size);
		virtual size_t Read(size_t offset, size_t size, uint8_t *out) override;
		virtual size_t Size() override;
	 private:
		std::shared_ptr<ProcessFile> file;
		size_t file_offset;
		size_t size;
	};
	class MemorySegment : public Segment {
	 public:
		MemorySegment(uint8_t *buffer, size_t size);
		virtual size_t Read(size_t offset, size_t size, uint8_t *out) override;
		virtual size_t Size() override;
	 private:
		uint8_t *buffer;
		size_t size;
	};

 protected:
	void SetSegments(std::vector<std::unique_ptr<Segment>> &&segments);

	std::vector<std::unique_ptr<Segment>> segments;
};

} // namespace fs
} // namespace process
} // namespace twili
