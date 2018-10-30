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

#include "ITwibPipeReader.hpp"

#include "err.hpp"

using trn::ResultCode;
using trn::ResultError;

namespace twili {
namespace bridge {

ITwibPipeReader::ITwibPipeReader(uint32_t device_id, std::weak_ptr<TwibPipe> pipe) : Object(device_id), pipe(pipe) {
}

void ITwibPipeReader::HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	switch((protocol::ITwibPipeReader::Command) command_id) {
	case protocol::ITwibPipeReader::Command::READ:
		Read(payload, opener);
		break;
	default:
		opener.BeginError(ResultCode(TWILI_ERR_PROTOCOL_UNRECOGNIZED_FUNCTION)).Finalize();
		break;
	}
}

void ITwibPipeReader::Read(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() != 0) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}

	if(std::shared_ptr<TwibPipe> observe = pipe.lock()) {
		observe->Read(
			[opener](uint8_t *data, size_t actual_size) mutable {
				if(actual_size == 0) {
					opener.BeginError(TWILI_ERR_EOF).Finalize();
				} else {
					auto r = opener.BeginOk(actual_size);
					r.Write(data, actual_size);
					r.Finalize();
				}
				return actual_size;
			});
	} else {
		opener.BeginError(TWILI_ERR_EOF).Finalize();
	}
}

} // namespace bridge
} // namespace twili
