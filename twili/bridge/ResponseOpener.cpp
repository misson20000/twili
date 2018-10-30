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

#include "ResponseOpener.hpp"

#include "err.hpp"

namespace twili {
namespace bridge {

using trn::ResultCode;
using trn::ResultError;

ResponseOpener::ResponseOpener(std::shared_ptr<detail::ResponseState> state) : state(state) {
}

ResponseWriter ResponseOpener::BeginError(ResultCode code, size_t payload_size, uint32_t object_count) const {
	if(state->has_begun) {
		throw ResultError(TWILI_ERR_FATAL_BRIDGE_STATE);
	}
	if(object_count * sizeof(uint32_t) > state->GetMaxTransferSize()) {
		throw ResultError(TWILI_ERR_FATAL_BRIDGE_STATE);
	}
	
	protocol::MessageHeader hdr;
	hdr.client_id = state->client_id;
	hdr.result_code = code.code;
	hdr.tag = state->tag;
	hdr.payload_size = payload_size;
	hdr.object_count = object_count;
	state->has_begun = true;
	state->total_size = payload_size;
	state->object_count = object_count;

	state->SendHeader(hdr);

	ResponseWriter writer(state);
	return writer;
}

ResponseWriter ResponseOpener::BeginOk(size_t payload_size, uint32_t object_count) const {
	return BeginError(ResultCode(0), payload_size, object_count);
}

} // namespace bridge
} // namespace twili
