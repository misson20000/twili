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

#include "NRONSOTransmutationFile.hpp"

#include<libtransistor/cpp/nx.hpp>

#include "../../twili.hpp"
#include "err.hpp"

namespace twili {
namespace process {
namespace fs {

static_assert(sizeof(NRONSOTransmutationFile::NROHeader) == 0x70, "nro header size should be 0x70");

trn::ResultCode NRONSOTransmutationFile::Create(std::shared_ptr<ProcessFile> nro, std::shared_ptr<NSOTransmutationFile> *out) {
	NROHeader header;
	TWILI_CHECK(header.Read(nro));

	*out = std::make_shared<NSOTransmutationFile>(
		std::make_unique<TransmutationFile::BackedSegment>(nro, header.segments[0].offset, header.segments[0].size),
		std::make_unique<TransmutationFile::BackedSegment>(nro, header.segments[1].offset, header.segments[1].size),
		std::make_unique<TransmutationFile::BackedSegment>(nro, header.segments[2].offset, header.segments[2].size),
		header.bss_size, header.build_id);

	return RESULT_OK;
}

trn::ResultCode NRONSOTransmutationFile::NROHeader::Read(std::shared_ptr<ProcessFile> nro) {
	uint64_t actual_size;
	if(nro->Read(0x10, sizeof(*this), (uint8_t*) this, &actual_size) != RESULT_OK ||
		 actual_size < sizeof(*this) ||
		 magic != 0x304f524e) {
		return TWILI_ERR_INVALID_NRO;
	} else {
		return RESULT_OK;
	}
}

} // namespace fs
} // namespace process
} // namespace twili
