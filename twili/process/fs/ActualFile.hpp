#pragma once

#include<stdio.h>

#include "ProcessFile.hpp"

namespace twili {
namespace process {
namespace fs {

class ActualFile : public ProcessFile {
 public:
	// takes ownership
	ActualFile(FILE *file);
	ActualFile(const char *path);
	virtual ~ActualFile() override;

	virtual size_t Read(size_t offset, size_t size, uint8_t *out) override;
	virtual size_t GetSize() override;
 private:
	FILE *file;
};

} // namespace fs
} // namespace process
} // namespace twili
