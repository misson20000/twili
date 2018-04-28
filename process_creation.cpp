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

Transistor::Result<std::shared_ptr<Transistor::KProcess>> CreateProcessFromNRO(std::vector<uint8_t> nro_image, const char *name, std::vector<uint32_t> caps) {
	try {
		if(nro_image.size() < sizeof(NroHeader)) {
			return tl::make_unexpected(TWILI_ERR_INVALID_NRO);
		}
		NroHeader *nro_header = (NroHeader*) nro_image.data();
		if(nro_header->magic != 0x304f524e) {
			return tl::make_unexpected(TWILI_ERR_INVALID_NRO);
		}
		if(nro_image.size() < nro_header->size) {
			return tl::make_unexpected(TWILI_ERR_INVALID_NRO);
		}
		
		const uint64_t nro_base = 0x7100000000;
		
		ProcessInfo process_info = {
			.process_category = 0,
			.title_id = 0x0100000000000036, // creport
			.code_addr = nro_base,
			.code_num_pages = (nro_header->size + nro_header->bss_size + 0xFFF) / 0x1000,
			.process_flags = 0b100111, // ASLR, 39-bit address space, AArch64
			.reslimit_h = 0,
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
		
		// load NRO
		{
			std::shared_ptr<Transistor::SVC::MemoryMapping> map =
				Transistor::ResultCode::AssertOk(
					Transistor::SVC::MapProcessMemory(proc, nro_base, nro_header->size + nro_header->bss_size));
			printf("Mapped at %p\n", map->Base());
			
			memcpy(map->Base(), nro_image.data(), nro_image.size());
			printf("Copied NRO\n");
		} // let map go out of scope
		
		printf("Reprotecting...\n");
		Transistor::ResultCode::AssertOk(
			Transistor::SVC::SetProcessMemoryPermission(*proc, nro_base + nro_header->segments[0].file_offset, nro_header->segments[0].size, 5)); // .text is RX
		Transistor::ResultCode::AssertOk(
			Transistor::SVC::SetProcessMemoryPermission(*proc, nro_base + nro_header->segments[1].file_offset, nro_header->segments[1].size, 1)); // .rodata is R
		Transistor::ResultCode::AssertOk(
			Transistor::SVC::SetProcessMemoryPermission(*proc, nro_base + nro_header->segments[2].file_offset, nro_header->segments[2].size + nro_header->bss_size, 3)); // .data + .bss is RW

		return proc;
	} catch(Transistor::ResultError e) {
		return tl::make_unexpected(e.code);
	}
}

}
}
