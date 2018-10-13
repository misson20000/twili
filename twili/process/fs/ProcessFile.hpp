#pragma once

#include<stdlib.h>
#include<stdint.h>

namespace twili {
namespace process {
namespace fs {

class ProcessFile {
 public:
	virtual ~ProcessFile() = default;
	virtual size_t Read(size_t offset, size_t size, uint8_t *out) = 0;
	virtual size_t GetSize() = 0;
};

} // namespace fs
} // namespace process
} // namespace twili
