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

#include<libtransistor/cpp/nx.hpp>
#include "NRONSOTransmutationFile.hpp"
#include "err.hpp"

namespace twili {
namespace process {
namespace fs {

static_assert(sizeof(NRONSOTransmutationFile::NROHeader) == 0x70, "nro header size should be 0x70");

std::shared_ptr<NSOTransmutationFile> NRONSOTransmutationFile::Create(std::shared_ptr<ProcessFile> nro) {
	NROHeader header(nro);
	return std::make_shared<NSOTransmutationFile>(
		std::make_unique<TransmutationFile::BackedSegment>(nro, header.segments[0].offset, header.segments[0].size),
		std::make_unique<TransmutationFile::BackedSegment>(nro, header.segments[1].offset, header.segments[1].size),
		std::make_unique<TransmutationFile::BackedSegment>(nro, header.segments[2].offset, header.segments[2].size),
		header.bss_size, header.build_id);
}

NRONSOTransmutationFile::NROHeader::NROHeader(std::shared_ptr<ProcessFile> nro) {
	if(nro->Read(0x10, sizeof(*this), (uint8_t*) this) < sizeof(*this)) {
		throw trn::ResultError(TWILI_ERR_IO_ERROR);
	}
	if(magic != 0x304f524e) {
		throw trn::ResultError(TWILI_ERR_INVALID_NRO);
	}
}

} // namespace fs
} // namespace process
} // namespace twili
