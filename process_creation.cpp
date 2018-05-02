#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/util.h>
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
	uint32_t personal_mm_heap_num_pages;
};

ProcessBuilder::Segment::Segment(std::vector<uint8_t> data, size_t data_offset, size_t data_length, size_t virt_length, uint32_t permissions) :
	data(data), data_offset(data_offset), data_length(data_length), virt_length(virt_length), permissions(permissions) {
}

ProcessBuilder::ProcessBuilder(const char *name, std::vector<uint32_t> caps) :
	name(name), caps(caps) {
	
}

Transistor::Result<uint64_t> ProcessBuilder::AppendSegment(Segment &&seg) {
	if((seg.virt_length & 0xFFF) != 0) {
		return tl::make_unexpected(TWILI_ERR_INVALID_SEGMENT);
	}
	if(seg.data.size() < seg.data_offset + seg.data_length) {
		return tl::make_unexpected(TWILI_ERR_INVALID_SEGMENT);
	}
	seg.load_addr = load_base + total_size;
	total_size+= seg.virt_length;
	segments.push_back(seg);
	return seg.load_addr;
}

Transistor::Result<uint64_t> ProcessBuilder::AppendNRO(std::vector<uint8_t> nro) {
	if(nro.size() < sizeof(NroHeader)) {
		return tl::make_unexpected(TWILI_ERR_INVALID_NRO);
	}
	NroHeader *nro_header = (NroHeader*) nro.data();
	if(nro_header->magic != 0x304f524e) {
		return tl::make_unexpected(TWILI_ERR_INVALID_NRO);
	}
	if(nro.size() < nro_header->size) {
		return tl::make_unexpected(TWILI_ERR_INVALID_NRO);
	}
	uint64_t base;
	Transistor::Result<uint64_t> r;

	r = AppendSegment(Segment(nro, nro_header->segments[0].file_offset, nro_header->segments[0].size, nro_header->segments[0].size, 5)); // .text is RX
	if(!r) { return r; }
	if(r) { base = *r; }
	r = AppendSegment(Segment(nro, nro_header->segments[1].file_offset, nro_header->segments[1].size, nro_header->segments[1].size, 1)); // .rodata is R
	if(!r) { return r; }
	r = AppendSegment(Segment(nro, nro_header->segments[2].file_offset, nro_header->segments[2].size, nro_header->segments[2].size + nro_header->bss_size, 3)); // .data + .bss is RW
	if(!r) { return r; }
	return base;
}

Transistor::Result<std::shared_ptr<Transistor::KProcess>> ProcessBuilder::Build() {
	try {
		Transistor::KResourceLimit resource_limit =
			Transistor::ResultCode::AssertOk(
				Transistor::SVC::CreateResourceLimit());

		Transistor::ResultCode::AssertOk(               // raise memory limit to 256MiB,
			Transistor::SVC::SetResourceLimitLimitValue(  // since the default limit of 6MiB
				resource_limit,                             // is too low for my tastes.
				Transistor::SVC::LimitableResource::Memory,
				1 * 256 * 1024 * 1024));

		Transistor::ResultCode::AssertOk(
			Transistor::SVC::SetResourceLimitLimitValue(
				resource_limit,
				Transistor::SVC::LimitableResource::Threads,
				256));

		Transistor::ResultCode::AssertOk(
			Transistor::SVC::SetResourceLimitLimitValue(
				resource_limit,
				Transistor::SVC::LimitableResource::Events,
				256));

		Transistor::ResultCode::AssertOk(
			Transistor::SVC::SetResourceLimitLimitValue(
				resource_limit,
				Transistor::SVC::LimitableResource::TransferMemories,
				256));
		
		Transistor::ResultCode::AssertOk(
			Transistor::SVC::SetResourceLimitLimitValue(
				resource_limit,
				Transistor::SVC::LimitableResource::Sessions,
				256));

		ProcessInfo process_info = {
			.process_category = 0,
			.title_id = 0x0100000000000036, // creport
			.code_addr = load_base,
			.code_num_pages = (uint32_t) ((total_size + 0xFFF) / 0x1000),
			.process_flags = 0b110111, // ASLR, 39-bit address space, AArch64, bit4 (?)
			.reslimit_h = resource_limit.handle,
			.system_resource_num_pages = 0,
			.personal_mm_heap_num_pages = 0,
		};
		strncpy((char*) process_info.name, name, sizeof(process_info).name-1);
		process_info.name[sizeof(process_info).name-1] = 0;
		
		// create process
		printf("Making process\n");
		auto proc = std::make_shared<Transistor::KProcess>(
			std::move(Transistor::ResultCode::AssertOk(
									Transistor::SVC::CreateProcess(&process_info, (void*) caps.data(), caps.size()))));
		printf("Made process 0x%x\n", proc->handle);
		
		// load segments
		{
			std::shared_ptr<Transistor::SVC::MemoryMapping> map =
				Transistor::ResultCode::AssertOk(
					Transistor::SVC::MapProcessMemory(proc, load_base, total_size));
			printf("Mapped at %p\n", map->Base());
			for(auto i = segments.begin(); i != segments.end(); i++) {
				memcpy(map->Base() + (i->load_addr - load_base), i->data.data() + i->data_offset, i->data_length);
			}
			printf("Copied segments\n");
		} // let map go out of scope
		
		printf("Reprotecting...\n");
		for(auto i = segments.begin(); i != segments.end(); i++) {
			Transistor::ResultCode::AssertOk(
				Transistor::SVC::SetProcessMemoryPermission(*proc, i->load_addr, i->virt_length, i->permissions));
		}

		return proc;
	} catch(Transistor::ResultError e) {
		return tl::make_unexpected(e.code);
	}
}

}
}
