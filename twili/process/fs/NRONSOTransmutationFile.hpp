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

#include "NSOTransmutationFile.hpp"

namespace twili {
namespace process {
namespace fs {

class NRONSOTransmutationFile {
 public:
	NRONSOTransmutationFile() = delete;
	static trn::ResultCode Create(std::shared_ptr<ProcessFile> nro, std::shared_ptr<NSOTransmutationFile> *out);

	struct NROSegmentHeader {
		uint32_t offset;
		uint32_t size;
	};
	struct NROHeader {
		trn::ResultCode Read(std::shared_ptr<ProcessFile> nro);
		
		uint32_t magic;
		uint32_t version;
		uint32_t size;
		uint32_t flags;
		NROSegmentHeader segments[3];
		uint32_t bss_size;
		uint32_t reserved;
		uint8_t build_id[0x20];
		uint64_t reserved2;
		uint64_t sections[3];
	} header;
};

} // namespace fs
} // namespace process
} // namespace twili
