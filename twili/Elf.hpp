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

namespace ELF {

enum {
	ELFCLASS32 = 1,
	ELFCLASS64 = 2,
};

enum {
	ELFDATALSB = 1,
	ELFDATAMSB = 2,
};

enum {
	ELFOSABI_SYSV = 0,
	ELFOSABI_HPUX = 1,
	ELFOSABI_STANDALONE = 255,
};

enum {
	ET_NONE = 0,
	ET_REL = 1,
	ET_EXEC = 2,
	ET_DYN = 3,
	ET_CORE = 4,
	ET_LOOS = 0xFE00,
	ET_HIOS = 0xFEFF,
	ET_LOPROC = 0xFF00,
	ET_HIPROC = 0xFFFF,
};

enum {
	EM_AARCH64 = 0xB7,
};

enum {
	PT_NULL = 0,
	PT_LOAD = 1,
	PT_DYNAMIC = 2,
	PT_INTERP = 3,
	PT_NOTE = 4,
};

enum {
	PF_X = 1,
	PF_W = 2,
	PF_R = 4
};

enum {
	NT_PRSTATUS = 1,
	NT_PRFPREG = 2,
	NT_PRPSINFO = 3,
	NT_TASKSTRUCT = 4,
	NT_AUXV = 6,

	NT_TWILI_PROCESS = 6480,
	NT_TWILI_THREAD = 6481,
	NT_TWILI_NSO = 6482,
};

enum {
	AT_NULL = 0, // end of vector
	AT_IGNORE = 1, // entry should be ignored
	AT_ENTRY = 9, // entry point for program
};

struct Elf64_Phdr {
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
};

struct Elf64_Shdr {
	uint32_t sh_name;
	uint32_t sh_type;
	uint64_t sh_flags;
	uint64_t sh_addr;
	uint64_t sh_offset;
	uint64_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint64_t sh_addralign;
	uint64_t sh_entsize;
};

struct Elf64_Ehdr {
	struct {
		uint8_t ei_mag[4] = {0x7f, 'E', 'L', 'F'};
		uint8_t ei_class;
		uint8_t ei_data;
		uint8_t ei_version;
		uint8_t ei_osabi;
		uint8_t ei_abiversion;
		uint8_t ei_pad[7];
	} e_ident;
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff; // program header offset
	uint64_t e_shoff; // section header offset
	uint32_t e_flags; // processor-flags
	uint16_t e_ehsze = sizeof(Elf64_Ehdr); // elf header size
	uint16_t e_phentsize = sizeof(Elf64_Phdr); // program header table entry size
	uint16_t e_phnum; // number of program headers
	uint16_t e_shentsize = sizeof(Elf64_Shdr); // section header table entry size
	uint16_t e_shnum; // number of section header table entries
	uint16_t e_shstrndx; // section header table index of section name string table section
};

namespace Note {

struct elf_prpsinfo {
	uint8_t pr_state;
	uint8_t pr_sname;
	uint8_t pr_zomb;
	uint8_t pr_nice;
	uint64_t pr_flag;
	uint32_t pr_uid;
	uint32_t pr_gid;
	uint32_t pr_pid;
	uint32_t pr_ppid;
	uint32_t pr_pgrp;
	uint32_t pr_sid;
	uint8_t pr_fname[16];
	uint8_t pr_psargs[80];
};

struct elf_siginfo {
	int32_t si_signo;
	int32_t si_code;
	int32_t si_errno;
};

struct elf_prstatus {
	elf_siginfo pr_info;
	int16_t pr_cursig;
	uint64_t pr_sigpend;
	uint64_t pr_sighold;
	uint32_t pr_pid;
	uint32_t pr_ppid;
	uint32_t pr_pgrp;
	uint32_t pr_sid;
	uint64_t times[8];
	uint64_t pr_reg[34]; // x0-x30, sp, pc, pstate
	uint32_t pr_fpvalid;
};
	
struct elf_auxv_t {
	uint64_t a_type;
	union {
		uint64_t a_val;
	} a_un;
};

struct twili_process {
	uint64_t title_id;
	uint64_t process_id;
	char process_name[12];
	uint32_t mmu_flags;
};

struct twili_thread {
	uint64_t thread_id;
	uint64_t tls_pointer;
	uint64_t entrypoint;
};

struct twili_nso_info {
	uint64_t addr;
	uint64_t size;
	uint8_t build_id[0x20];
};

}

}
