#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/alloc_pages.h>
#include<libtransistor/util.h>
#include<libtransistor/svc.h>

#include<stdio.h>
#include<errno.h>

#include<optional>
#include<tuple>

std::optional<std::tuple<void*, size_t>> ReadTwili() {
	FILE *twili_f = fopen("/squash/twili.nro", "rb");
	if(twili_f == NULL) {
		printf("Failed to open /squash/twili.nro\n");
		return std::nullopt;
	}

	if(fseek(twili_f, 0, SEEK_END) != 0) {
		printf("Failed to SEEK_END\n");
		fclose(twili_f);
		return std::nullopt;
	}

	ssize_t twili_size = ftell(twili_f);
	if(twili_size == -1) {
		printf("Failed to ftell\n");
		fclose(twili_f);
		return std::nullopt;
	}

	if(fseek(twili_f, 0, SEEK_SET) != 0) {
		printf("Failed to SEEK_SET\n");
		fclose(twili_f);
		return std::nullopt;
	}

	void *twili_buffer = alloc_pages(twili_size, twili_size, NULL);
	if(twili_buffer == NULL) {
		printf("Failed to allocate space for Twili\n");
		fclose(twili_f);
		return std::nullopt;
	}

	ssize_t total_read = 0;
	while(total_read < twili_size) {
		size_t r = fread((uint8_t*) twili_buffer + total_read, 1, twili_size - total_read, twili_f);
		if(r == 0) {
			printf("Failed to read Twili (read 0x%lx/0x%lx bytes): %d\n", total_read, twili_size, errno);
			free_pages(twili_buffer);
			fclose(twili_f);
			return std::nullopt;
		}
		total_read+= r;
	}
	fclose(twili_f);

	return std::make_tuple(twili_buffer, twili_size);
}

int main(int argc, char *argv[]) {
	printf("===TWILI LAUNCHER===\n");

	if(auto twili_tuple = ReadTwili()) {
		void *twili_image = std::get<0>(*twili_tuple);
		size_t twili_size = std::get<1>(*twili_tuple);
		
		printf("Read Twili (0x%lx bytes)\n", twili_size);

		try {
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

			NroHeader *twili_header = (NroHeader*) twili_image;

			const uint64_t twili_base = 0x7100000000;

			struct {
				uint8_t name[12];
				uint32_t process_category;
				uint64_t title_id;
				uint64_t code_addr;
				uint32_t code_num_pages;
				uint32_t process_flags;
				handle_t reslimit_h;
				uint32_t system_resource_num_pages;
				uint32_t personal_mm_heap_num_pages;
			} process_info = {
				.name = "twili",
				.process_category = 0,
				.title_id = 0x0100000000000036, // creport
				.code_addr = twili_base,
				.code_num_pages = (twili_header->size + twili_header->bss_size + 0xFFF) / 0x1000,
				.process_flags = 0b100111, // ASLR, 39-bit address space, AArch64
				.reslimit_h = 0,
				.system_resource_num_pages = 0,
				.personal_mm_heap_num_pages = 0,
			};
			
			uint32_t caps[] = {
				0b00011111111111111111111111101111,
				0b00111111111111111111111111101111,
				0b01011111111111111111111111101111,
				0b01100000000000001111111111101111,
				0b10011111100000000000000000001111,
				0b10100000000000000000000000101111,
				0b00000010000000000111001110110111,
				0b00000000000000000101111111111111,
				0b00000000000110000011111111111111,
				0b00000010000000000111111111111111,
				0b00000000000000001111111111111111
			};

			// create Twili process
			printf("Making process\n");
			auto proc = std::make_shared<Transistor::KProcess>(
				std::move(Transistor::ResultCode::AssertOk(
										Transistor::SVC::CreateProcess(&process_info, (void*) caps, ARRAY_LENGTH(caps)))));
			printf("Made process 0x%x\n", proc->handle);

			// load Twili

			{
				std::shared_ptr<Transistor::SVC::MemoryMapping> map =
					Transistor::ResultCode::AssertOk(
						Transistor::SVC::MapProcessMemory(proc, twili_base, twili_header->size + twili_header->bss_size));
				printf("Mapped at %p\n", map->Base());
				
				memcpy(map->Base(), twili_image, twili_size);
				printf("Copied Twili\n");
			} // let map go out of scope

			printf("Reprotecting...\n");
			Transistor::ResultCode::AssertOk(
				Transistor::SVC::SetProcessMemoryPermission(*proc, twili_base + twili_header->segments[0].file_offset, twili_header->segments[0].size, 5)); // .text is RX
			Transistor::ResultCode::AssertOk(
				Transistor::SVC::SetProcessMemoryPermission(*proc, twili_base + twili_header->segments[1].file_offset, twili_header->segments[1].size, 1)); // .rodata is R
			Transistor::ResultCode::AssertOk(
				Transistor::SVC::SetProcessMemoryPermission(*proc, twili_base + twili_header->segments[2].file_offset, twili_header->segments[2].size + twili_header->bss_size, 3)); // .data + .bss is RW
			
			// launch Twili
			printf("Launching...\n");
			Transistor::ResultCode::AssertOk(
				Transistor::SVC::StartProcess(*proc, 58, 0, 0x100000));

			printf("Launched\n");
			svcSleepThread(1000000000);
			printf("Exiting...\n");
		} catch(Transistor::ResultError e) {
			printf("caught ResultError: %s\n", e.what());
		}
	} else {
		return 1;
	}
	
	return 0;
}
