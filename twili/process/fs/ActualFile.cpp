#include "ActualFile.hpp"

#include<libtransistor/cpp/nx.hpp>

#include "err.hpp"

namespace twili {
namespace process {
namespace fs {

ActualFile::ActualFile(FILE *file) : file(file) {
	if(file == NULL) {
		printf("failed to open file\n");
		throw trn::ResultError(TWILI_ERR_IO_ERROR);
	}
}

ActualFile::ActualFile(const char *path) : file(fopen(path, "rb")) {
	if(file == NULL) {
		printf("failed to open '%s'\n", path);
		throw trn::ResultError(TWILI_ERR_IO_ERROR);
	}
}

ActualFile::~ActualFile() {
	if(file != NULL) {
		fclose(file);
	}
}

size_t ActualFile::Read(size_t offset, size_t size, uint8_t *out) {
	if(fseek(file, offset, SEEK_SET) != 0) {
		throw trn::ResultError(TWILI_ERR_IO_ERROR);
	}
	return fread((void*) out, 1, size, file);
}

size_t ActualFile::GetSize() {
	if(fseek(file, 0, SEEK_END) != 0) {
		throw trn::ResultError(TWILI_ERR_IO_ERROR);
	}
	off_t out = ftello(file);
	if(out == -1) {
		throw trn::ResultError(TWILI_ERR_IO_ERROR);
	}
	return out;
}

} // namespace fs
} // namespace process
} // namespace twili
