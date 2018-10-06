#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/util.h>
#include<libtransistor/environment.h>
#include<libtransistor/svc.h>

#include "process_creation.hpp"
#include "err.hpp"

namespace twili {
namespace process_creation {

struct SegmentHeader {
	uint32_t file_offset;
	uint32_t size;
};

struct NroHeader {
	uint32_t jump;
	uint32_t mod_offset;
	uint32_t padding1[2];
	uint32_t magic;
	uint32_t unknown1;
	uint32_t size;
	uint32_t unknown2;
	SegmentHeader segments[3];
	uint32_t bss_size;
};

struct ProcessInfo {
	uint8_t name[12];
	uint32_t process_category;
	uint64_t title_id;
	uint64_t code_addr;
	uint32_t code_num_pages;
	uint32_t process_flags;
	handle_t reslimit_h;
	uint32_t system_resource_num_pages;
};

ProcessBuilder::Segment::Segment(DataReader &data, size_t data_offset, size_t data_length, size_t virt_length, uint32_t permissions) :
	data(data), data_offset(data_offset), data_length(data_length), virt_length(virt_length), permissions(permissions) {
}

ProcessBuilder::FileDataReader::FileDataReader(FILE *f) : f(f) {
}

void ProcessBuilder::FileDataReader::Seek(size_t pos) {
	fseek(f, pos, SEEK_SET);
}

void ProcessBuilder::FileDataReader::Read(uint8_t *target, size_t size) {
	ssize_t r;
	while((r = fread(target, 1, size, f)) < size) {
		if(r > 0) {
			target+= r;
			size-= r;
		} else {
			abort();
		}
	}
}

size_t ProcessBuilder::FileDataReader::Tell() {
	return ftell(f);
}

size_t ProcessBuilder::FileDataReader::TotalSize() {
	size_t pos = ftell(f);
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, pos, SEEK_SET);
	return size;
}

ProcessBuilder::VectorDataReader::VectorDataReader(std::vector<uint8_t> vec) : vec(vec) {
}

void ProcessBuilder::VectorDataReader::Seek(size_t pos) {
	this->pos = pos;
}

void ProcessBuilder::VectorDataReader::Read(uint8_t *target, size_t size) {
	memcpy(target, vec.data() + pos, size);
	pos+= size;
}

size_t ProcessBuilder::VectorDataReader::Tell() {
	return pos;
}

size_t ProcessBuilder::VectorDataReader::TotalSize() {
	return vec.size();
}

ProcessBuilder::ProcessBuilder() {
	if(env_get_kernel_version() >= KERNEL_VERSION_200) {
		load_base = 0x7100000000;
	} else {
		load_base = 0x71000000;
	}
}

trn::Result<uint64_t> ProcessBuilder::AppendSegment(Segment &&seg) {
	if((seg.virt_length & 0xFFF) != 0) {
		return tl::make_unexpected(TWILI_ERR_INVALID_SEGMENT);
	}
	if(seg.data.TotalSize() < seg.data_offset + seg.data_length) {
		return tl::make_unexpected(TWILI_ERR_INVALID_SEGMENT);
	}
	seg.load_addr = load_base + total_size;
	total_size+= seg.virt_length;
	segments.push_back(seg);
	return seg.load_addr;
}

trn::Result<uint64_t> ProcessBuilder::AppendNRO(DataReader &nro) {
	if(nro.TotalSize() < sizeof(NroHeader)) {
		return tl::make_unexpected(TWILI_ERR_INVALID_NRO);
	}
	NroHeader nro_header;
	nro.Read((uint8_t*) &nro_header, sizeof(nro_header));
	if(nro_header.magic != 0x304f524e) {
		return tl::make_unexpected(TWILI_ERR_INVALID_NRO);
	}
	if(nro.TotalSize() < nro_header.size) {
		return tl::make_unexpected(TWILI_ERR_INVALID_NRO);
	}
	uint64_t base;
	trn::Result<uint64_t> r;

	r = AppendSegment(Segment(nro, nro_header.segments[0].file_offset, nro_header.segments[0].size, nro_header.segments[0].size, 5)); // .text is RX
	if(!r) { return r; }
	if(r) { base = *r; }
	r = AppendSegment(Segment(nro, nro_header.segments[1].file_offset, nro_header.segments[1].size, nro_header.segments[1].size, 1)); // .rodata is R
	if(!r) { return r; }
	r = AppendSegment(Segment(nro, nro_header.segments[2].file_offset, nro_header.segments[2].size, nro_header.segments[2].size + nro_header.bss_size, 3)); // .data + .bss is RW
	if(!r) { return r; }
	return base;
}

trn::Result<std::nullopt_t> ProcessBuilder::Load(std::shared_ptr<trn::KProcess> proc, uint64_t load_base) {
	// load segments
	{
		std::shared_ptr<trn::svc::MemoryMapping> map =
			trn::ResultCode::AssertOk(
				trn::svc::MapProcessMemory(proc, load_base, total_size));
		printf("Mapped at %p\n", map->Base());
		for(auto i = segments.begin(); i != segments.end(); i++) {
			uint8_t *target = map->Base() + (i->load_addr - this->load_base);
			i->data.Seek(i->data_offset);
			i->data.Read(target, i->data_length);
		}
		printf("Copied segments\n");
	} // let map go out of scope

	printf("Reprotecting...\n");
	for(auto i = segments.begin(); i != segments.end(); i++) {
		trn::ResultCode::AssertOk(
			trn::svc::SetProcessMemoryPermission(*proc, load_base + (i->load_addr - this->load_base), i->virt_length, i->permissions));
	}

	return std::nullopt;
}

trn::Result<std::shared_ptr<trn::KProcess>> ProcessBuilder::Build(const char *name, std::vector<uint32_t> caps) {
	try {
		trn::KResourceLimit resource_limit =
			trn::ResultCode::AssertOk(
				trn::svc::CreateResourceLimit());

		trn::ResultCode::AssertOk(               // raise memory limit to 256MiB,
			trn::svc::SetResourceLimitLimitValue(  // since the default limit of 6MiB
				resource_limit,                             // is too low for my tastes.
				trn::svc::LimitableResource::Memory,
				1 * 256 * 1024 * 1024));

		trn::ResultCode::AssertOk(
			trn::svc::SetResourceLimitLimitValue(
				resource_limit,
				trn::svc::LimitableResource::Threads,
				256));

		trn::ResultCode::AssertOk(
			trn::svc::SetResourceLimitLimitValue(
				resource_limit,
				trn::svc::LimitableResource::Events,
				256));

		trn::ResultCode::AssertOk(
			trn::svc::SetResourceLimitLimitValue(
				resource_limit,
				trn::svc::LimitableResource::TransferMemories,
				256));
		
		trn::ResultCode::AssertOk(
			trn::svc::SetResourceLimitLimitValue(
				resource_limit,
				trn::svc::LimitableResource::Sessions,
				256));

		enum {
			PROCESS_FLAG_AARCH64 = 0b1,
			PROCESS_FLAG_AS_32BIT = 0b0000,
			PROCESS_FLAG_AS_36BIT = 0b0010,
			PROCESS_FLAG_AS_32BIT_NO_MAP = 0b0100,
			PROCESS_FLAG_AS_39BIT = 0b0110,
			PROCESS_FLAG_ENABLE_DEBUG = 0b10000,
			PROCESS_FLAG_ENABLE_ASLR = 0b100000,
			PROCESS_FLAG_USE_SYSTEM_MEM_BLOCKS = 0b1000000,
		};

		uint32_t process_flags =
			PROCESS_FLAG_AARCH64 |
			PROCESS_FLAG_ENABLE_DEBUG |
			PROCESS_FLAG_ENABLE_ASLR;

		if(env_get_kernel_version() >= KERNEL_VERSION_200) {
			process_flags|= PROCESS_FLAG_AS_39BIT;
		} else {
			process_flags|= PROCESS_FLAG_AS_36BIT;
		}
		
		ProcessInfo process_info = {
			.process_category = 0,
			.title_id = 0x0100000000006481,
			.code_addr = load_base,
			.code_num_pages = (uint32_t) ((total_size + 0xFFF) / 0x1000),
			.process_flags = process_flags,
			.reslimit_h = resource_limit.handle,
			.system_resource_num_pages = 0,
		};
		strncpy((char*) process_info.name, name, sizeof(process_info).name-1);
		process_info.name[sizeof(process_info).name-1] = 0;
		
		// create process
		printf("Making process (base addr 0x%lx)\n", load_base);
		auto proc = std::make_shared<trn::KProcess>(
			std::move(trn::ResultCode::AssertOk(
									trn::svc::CreateProcess(&process_info, (void*) caps.data(), caps.size()))));
		printf("Made process 0x%x\n", proc->handle);

		return Load(proc, load_base).map([proc](auto _) { return proc; });
	} catch(trn::ResultError e) {
		return tl::make_unexpected(e.code);
	}
}

size_t ProcessBuilder::GetTotalSize() {
	return total_size;
}

} // namespace process_creation
} // namespace twili
