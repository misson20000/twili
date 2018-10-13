#pragma once

#include<vector>

#include "ProcessFile.hpp"

namespace twili {
namespace process {
namespace fs {

class VectorFile : public ProcessFile {
 public:
	VectorFile(std::vector<uint8_t> vector);
	virtual ~VectorFile() override = default;
	
	virtual size_t Read(size_t offset, size_t size, uint8_t *out) override;
	virtual size_t GetSize() override;
 private:
	std::vector<uint8_t> vector;
};

} // namespace fs
} // namespace process
} // namespace twili
