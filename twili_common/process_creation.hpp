#pragma once

#include<libtransistor/cpp/types.hpp>

#include<vector>
#include<memory>

namespace twili {
namespace process_creation {

class ProcessBuilder {
 public:
	class DataReader {
		public:
		virtual void Seek(size_t pos) = 0;
		virtual void Read(uint8_t *target, size_t size) = 0;
		virtual size_t Tell() = 0;
		virtual size_t TotalSize() = 0;
	};
	class VectorDataReader : public DataReader {
		public:
		VectorDataReader(std::vector<uint8_t> vec);
		virtual void Seek(size_t pos);
		virtual void Read(uint8_t *target, size_t size);
		virtual size_t Tell();
		virtual size_t TotalSize();
		private:
		std::vector<uint8_t> vec;
		size_t pos = 0;
	};
	class FileDataReader : public DataReader {
		public:
		FileDataReader(FILE *f);
		virtual void Seek(size_t pos);
		virtual void Read(uint8_t *target, size_t size);
		virtual size_t Tell();
		virtual size_t TotalSize();
		private:
		FILE *f;
	};
	class Segment {
	 public:
		Segment(DataReader &data, size_t data_offset, size_t data_length, size_t virt_length, uint32_t permissions);
		DataReader &data;
		size_t data_offset; // offset into data that this segment starts
		size_t data_length;
		size_t virt_length;
		uint32_t permissions;
		uint64_t load_addr;
	};
	
	ProcessBuilder(const char *name, std::vector<uint32_t> caps);

	// returns load address
	trn::Result<uint64_t> AppendSegment(Segment &&seg);
	// returns load address
	trn::Result<uint64_t> AppendNRO(DataReader &nro_reader);
	
	trn::Result<std::shared_ptr<trn::KProcess>> Build();
 private:
	const char *name;
	std::vector<uint32_t> caps;
	std::vector<Segment> segments;
	uint64_t load_base;
	size_t total_size = 0;
};

}
}
