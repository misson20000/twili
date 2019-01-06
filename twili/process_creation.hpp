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

#include<libtransistor/cpp/types.hpp>

#include<vector>
#include<memory>

#include "process/fs/ProcessFile.hpp"

namespace twili {
namespace process {

class ProcessBuilder {
 public:
	class Segment {
	 public:
		Segment(std::shared_ptr<fs::ProcessFile> file, size_t data_offset, size_t data_length, size_t virt_length, uint32_t permissions);
		std::shared_ptr<fs::ProcessFile> file;
		size_t data_offset; // offset into data that this segment starts
		size_t data_length;
		size_t virt_length;
		uint32_t permissions;
		uint64_t load_addr;
	};
	
	ProcessBuilder();

	// returns load address
	trn::Result<uint64_t> AppendSegment(Segment &&seg);
	// returns load address
	trn::Result<uint64_t> AppendNRO(std::shared_ptr<fs::ProcessFile> nro);

	trn::Result<std::nullopt_t> Load(std::shared_ptr<trn::KProcess> process, uint64_t base);
	trn::Result<std::shared_ptr<trn::KProcess>> Build(const char *name, std::vector<uint32_t> caps);

	size_t GetTotalSize();
 private:
	std::vector<Segment> segments;
	uint64_t load_base;
	size_t total_size = 0;
};

} // namespace process
} // namespace twili
