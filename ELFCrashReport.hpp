#pragma once

#include<libtransistor/cpp/types.hpp>

#include<iostream>
#include<fstream>
#include<vector>
#include<string>

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
	
 public:
	ELFCrashReport();
	void AddVMA(uint64_t virtual_addr, uint64_t size, uint32_t flags);
	void AddNote(std::string name, uint32_t type, std::vector<uint8_t> desc);

	template<typename T>
	void AddNote(std::string name, uint32_t type, T t) {
		static_assert(std::is_standard_layout<T>::value, "T must be standard layout");
		std::vector<uint8_t> bytes(sizeof(T), 0);
		memcpy(bytes.data(), &t, sizeof(T));
		AddNote(name, type, bytes);
	}
	
	Transistor::Result<std::nullopt_t> Generate(Transistor::KDebug &debug, usb::USBBridge::USBResponseWriter &writer);
	
 private:
	std::vector<VMA> vmas;
	std::vector<Note> notes;
};

}
