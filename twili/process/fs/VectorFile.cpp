#include "VectorFile.hpp"

namespace twili {
namespace process {
namespace fs {

VectorFile::VectorFile(std::vector<uint8_t> vector) : vector(vector) {
}

size_t VectorFile::Read(size_t offset, size_t size, uint8_t *out) {
	size_t actual_size = std::min(vector.size() - offset, size);
	std::copy_n(vector.begin() + offset, actual_size, out);
	return actual_size;
}

size_t VectorFile::GetSize() {
	return vector.size();
}

} // namespace fs
} // namespace process
} // namespace twili
