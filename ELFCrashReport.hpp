#pragma once

#include<libtransistor/cpp/types.hpp>

#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<map>

#include "Elf.hpp"
#include "USBBridge.hpp"

namespace twili {

class ELFCrashReport {
	struct VMA {
		uint64_t file_offset;
		uint64_t virtual_addr;
		size_t size;
		uint32_t flags;
	};

	struct Note {
		uint32_t namesz;
		uint32_t descsz;
		uint32_t type;
		std::vector<uint8_t> name;
		std::vector<uint8_t> desc;
	};

	class Thread {
	 public:
		Thread(uint64_t thread_id, uint64_t tls_pointer, uint64_t entrypoint);
		
		int signo = 0;
		ELF::Note::elf_prstatus GeneratePRSTATUS(trn::KDebug &debug);
	 private:
		uint64_t thread_id;
		uint64_t tls_pointer;
		uint64_t entrypoint;
	};
	
 public:
	ELFCrashReport();
	void AddVMA(uint64_t virtual_addr, uint64_t size, uint32_t flags);
	void AddThread(uint64_t thread_id, uint64_t tls_pointer, uint64_t entrypoint);
	Thread *GetThread(uint64_t thread_id);

	template<typename T>
	void AddNote(std::string name, uint32_t type, T t) {
		static_assert(std::is_standard_layout<T>::value, "T must be standard layout");
		std::vector<uint8_t> bytes(sizeof(T), 0);
		memcpy(bytes.data(), &t, sizeof(T));
		AddNote(name, type, bytes);
	}
	
	trn::Result<std::nullopt_t> Generate(trn::KDebug &debug, usb::USBBridge::USBResponseWriter &writer);
	void AddNote(std::string name, uint32_t type, std::vector<uint8_t> desc);
	
 private:
	std::vector<VMA> vmas;
	std::vector<Note> notes;
	std::map<uint64_t, Thread> threads;
};

}
