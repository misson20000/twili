#pragma once

#include<libtransistor/cpp/types.hpp>

#include<vector>
#include<memory>

namespace twili {
namespace process_creation {

class ProcessBuilder {
 public:
	class Segment {
	 public:
		Segment(std::vector<uint8_t> data, size_t data_offset, size_t data_length, size_t virt_length, uint32_t permissions);
		std::vector<uint8_t> data;
		size_t data_offset; // offset into data that this segment starts
		size_t data_length;
		size_t virt_length;
		uint32_t permissions;
		uint64_t load_addr;
	};
	
	ProcessBuilder(const char *name, std::vector<uint32_t> caps);

	// returns load address
	Transistor::Result<uint64_t> AppendSegment(Segment &&seg);
	// returns load address
	Transistor::Result<uint64_t> AppendNRO(std::vector<uint8_t> nro);
	
	Transistor::Result<std::shared_ptr<Transistor::KProcess>> Build();
 private:
	const char *name;
	std::vector<uint32_t> caps;
	std::vector<Segment> segments;
	uint64_t load_base = 0x7100000000;
	size_t total_size = 0;
};

}
}
