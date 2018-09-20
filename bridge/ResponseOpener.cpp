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
