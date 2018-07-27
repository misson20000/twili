#pragma once

#include<stdexcept>

namespace twili {
namespace twib {

class ResultError : public std::runtime_error {
 public:
	ResultError(uint32_t result);

	const char *what() noexcept;
	
	const uint32_t code;
 private:
	std::string description;
};

} // namespace twib
} // namespace twili
