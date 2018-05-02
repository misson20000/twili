#include "util.hpp"

#include<stdio.h>
#include<errno.h>

namespace twili {
namespace util {

std::optional<std::vector<uint8_t>> ReadFile(const char *path) {
	FILE *f = fopen(path, "rb");
	if(f == NULL) {
		printf("Failed to open %s\n", path);
		return std::nullopt;
	}

	if(fseek(f, 0, SEEK_END) != 0) {
		printf("Failed to SEEK_END\n");
		fclose(f);
		return std::nullopt;
	}

	ssize_t size = ftell(f);
	if(size == -1) {
		printf("Failed to ftell\n");
		fclose(f);
		return std::nullopt;
	}

	if(fseek(f, 0, SEEK_SET) != 0) {
		printf("Failed to SEEK_SET\n");
		fclose(f);
		return std::nullopt;
	}

	std::vector<uint8_t> buffer(size, 0);

	ssize_t total_read = 0;
	while(total_read < size) {
		size_t r = fread(buffer.data() + total_read, 1, size - total_read, f);
		if(r == 0) {
			printf("Failed to read (read 0x%lx/0x%lx bytes): %d\n", total_read, size, errno);
			fclose(f);
			return std::nullopt;
		}
		total_read+= r;
	}
	fclose(f);

	return buffer;
}

}
}
