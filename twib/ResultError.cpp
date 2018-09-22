#include "ResultError.hpp"

#include<sstream>

namespace twili {
namespace twib {

ResultError::ResultError(uint32_t code) : std::runtime_error("failed to format result code"), code(code) {
	std::ostringstream ss;
	ss << "ResultError: 0x" << std::hex << code;
	description = ss.str();
}

const char *ResultError::what() const noexcept {
	return description.c_str();
}

} // namespace twib
} // namespace twili
