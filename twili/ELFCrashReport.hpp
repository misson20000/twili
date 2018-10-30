//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2018 misson20000 <xenotoad@xenotoad.net>
//
// This file is part of Twili.
//
// Twili is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Twili is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Twili.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include<libtransistor/cpp/types.hpp>

#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<map>

#include "Elf.hpp"
#include "bridge/ResponseOpener.hpp"

namespace twili {
namespace process {
class Process;
}

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

	template<typename T>
	void AddNote(std::string name, uint32_t type, T t) {
		static_assert(std::is_standard_layout<T>::value, "T must be standard layout");
		std::vector<uint8_t> bytes(sizeof(T), 0);
		memcpy(bytes.data(), &t, sizeof(T));
		AddNote(name, type, bytes);
	}
	
	void Generate(process::Process &process, bridge::ResponseOpener opener);
	void AddNote(std::string name, uint32_t type, std::vector<uint8_t> desc);

	template<typename T>
	void AddNote(std::string name, uint32_t type, std::vector<T> desc) {
		static_assert(std::is_standard_layout<T>::value, "T must be standard layout");
		uint8_t *desc_bytes = (uint8_t*) desc.data();
		std::vector<uint8_t> bytes(desc_bytes, desc_bytes + (desc.size() * sizeof(T)));
		AddNote(name, type, bytes);
	}
	
 private:
	std::vector<VMA> vmas;
	std::vector<Note> notes;
	std::map<uint64_t, Thread> threads;

	void AddVMA(uint64_t virtual_addr, uint64_t size, uint32_t flags);
	void AddThread(uint64_t thread_id, uint64_t tls_pointer, uint64_t entrypoint);
	Thread *GetThread(uint64_t thread_id);
};

}
