#include "ResponseOpener.hpp"

#include "err.hpp"

namespace twili {
namespace bridge {

using trn::ResultCode;
using trn::ResultError;

ResponseWriter::ResponseWriter(std::shared_ptr<detail::ResponseState> state) : state(state) {
}

void ResponseWriter::Write(uint8_t *data, size_t size) {
	state->SendData(data, size);
}

void ResponseWriter::Write(const std::string &&str) {
	Write((uint8_t*) str.data(), str.size());
}

uint32_t ResponseWriter::Object(std::shared_ptr<bridge::Object> object) {
	size_t index = state->objects.size();
	state->objects.push_back(object);
	return index;
}

void ResponseWriter::Finalize() {
	state->Finalize();
}

} // namespace bridge
} // namespace twili
