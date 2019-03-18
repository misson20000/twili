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

#include "ITwibFileAccessor.hpp"

#include<libtransistor/cpp/svc.hpp>

#include "err.hpp"

#include<cstring>

using namespace trn;

namespace twili {
namespace bridge {

ITwibFileAccessor::ITwibFileAccessor(uint32_t object_id, ifile_t ifile) : ObjectDispatcherProxy(*this, object_id), ifile(ifile), dispatcher(*this) {
	
}

ITwibFileAccessor::~ITwibFileAccessor() {
	ipc_close(ifile);
}

void ITwibFileAccessor::Read(bridge::ResponseOpener opener, uint64_t offset, uint64_t size) {
	const size_t limit = 0x40000;

	std::vector<uint8_t> buffer(std::max(size, limit));
	size_t actual_size;

	ResultCode::AssertOk(ifile_read(ifile, &actual_size, buffer.data(), buffer.size(), 0, offset, buffer.size()));
	buffer.resize(actual_size);
	
	opener.RespondOk(std::move(buffer));
}

void ITwibFileAccessor::Write(bridge::ResponseOpener opener, uint64_t offset, InputStream &stream) {
	std::shared_ptr<uint64_t> offset_shared = std::make_shared<uint64_t>(offset);

	stream.receive =
		[this, offset_shared](util::Buffer &buffer) {
			size_t a = buffer.ReadAvailable();
			ResultCode::AssertOk(ifile_write(ifile, 0, *offset_shared, a, buffer.Read(), a));
			*offset_shared+= a;
		};

	stream.finish =
		[this, opener](util::Buffer &buffer) {
			// since our receiver always consumes the entire buffer, we
			// don't have to worry about finishing it.
			opener.RespondOk();
		};
}

void ITwibFileAccessor::Flush(bridge::ResponseOpener opener) {
	ResultCode::AssertOk(ifile_flush(ifile));
	opener.RespondOk();
}

void ITwibFileAccessor::SetSize(bridge::ResponseOpener opener, size_t size) {
	ResultCode::AssertOk(ifile_set_size(ifile, size));
	opener.RespondOk();
}

void ITwibFileAccessor::GetSize(bridge::ResponseOpener opener) {
	size_t size;
	ResultCode::AssertOk(ifile_get_size(ifile, &size));
	opener.RespondOk(std::move(size));
}

} // namespace bridge
} // namespace twili
