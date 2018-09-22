#pragma once

#include<stdexcept>
#include<string>

namespace twili {
namespace twib {

class ResultError : public std::runtime_error {
 public:
	ResultError(uint32_t result);

	virtual const char *what() const noexcept override;
	
	const uint32_t code;
 private:
	std::string description;
};

} // namespace twib
} // namespace twili
