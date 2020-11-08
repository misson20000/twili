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

#include "ELFCrashReport.hpp"

#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/util.h>

#include<vector>
#include<string>

#include "twili.hpp"
#include "process/Process.hpp"

using trn::ResultCode;

namespace twili {

ELFCrashReport::ELFCrashReport() {
}

void ELFCrashReport::AddVMA(uint64_t virtual_addr, uint64_t size, uint32_t flags) {
	vmas.push_back({0, virtual_addr, size, flags});
}

void ELFCrashReport::AddNote(std::string name, uint32_t type, std::vector<uint8_t> desc) {
	std::vector<uint8_t> name_vec(name.begin(), name.end());
	while(name_vec.size() % 4 != 0) {
		name_vec.push_back(0);
	}
	std::vector<uint8_t> desc_vec(desc);
	while(desc_vec.size() % 4 != 0) {
		desc_vec.push_back(0);
	}
	
	notes.push_back({static_cast<uint32_t>(name.length()), static_cast<uint32_t>(desc.size()), type, name_vec, desc_vec});
}

void ELFCrashReport::AddThread(uint64_t thread_id, uint64_t tls_pointer, uint64_t entrypoint) {
	threads.emplace(std::make_pair(thread_id, Thread(thread_id, tls_pointer, entrypoint)));
}

ELFCrashReport::Thread *ELFCrashReport::GetThread(uint64_t thread_id) {
	return &threads.find(thread_id)->second;
}

void ELFCrashReport::Generate(process::Process &process, twili::bridge::ResponseOpener ro) {
	process.AddNotes(*this);
	
	trn::KDebug debug = ({
			auto r = trn::svc::DebugActiveProcess(process.GetPid());
			if(!r) { ro.RespondError(r.error()); return; }
			std::move(*r); });
	
	printf("  opened debug: 0x%x\n", debug.handle);

	while(1) {
		auto r = trn::svc::GetDebugEvent(debug);
		if(!r) {
			if(r.error().code == 0x8c01) {
				break;
			} else {
				twili::Abort(r.error());
			}
		}

		const char *types[] = {"ATTACH_PROCESS", "ATTACH_THREAD", "UNKNOWN", "EXIT", "EXCEPTION"};
		printf("  Event (type=0x%x [%s])\n", r->event_type, types[r->event_type]);
		printf("    Flags: 0x%x\n", r->flags);
		printf("    Thread id: 0x%lx\n", r->thread_id);

		switch(r->event_type) {
		case DEBUG_EVENT_ATTACH_PROCESS: {
			printf("    AttachProcess:\n");
			printf("      Title ID: 0x%lx\n", r->attach_process.title_id);
			printf("      Process ID: 0x%lx\n", r->attach_process.process_id);
			printf("      Process Name: %s\n", r->attach_process.process_name);
			printf("      MMU flags: 0x%x\n", r->attach_process.mmu_flags);
			ELF::Note::elf_prpsinfo psinfo = {
				.pr_state = 0,
				.pr_sname = '.',
				.pr_zomb = 0,
				.pr_nice = 0,
				.pr_flag = 0,
				.pr_uid = 0,
				.pr_pid = (uint32_t) r->attach_process.process_id,
				.pr_ppid = 0,
				.pr_pgrp = 0,
				.pr_sid = 0,
				.pr_psargs = ""
			};
			memcpy(psinfo.pr_fname, r->attach_process.process_name, 12);
			AddNote<ELF::Note::elf_prpsinfo>("CORE", ELF::NT_PRPSINFO, psinfo);
			ELF::Note::twili_process twiproc = {
				.title_id = r->attach_process.title_id,
				.process_id = r->attach_process.process_id,
				.mmu_flags = r->attach_process.mmu_flags,
			};
			memcpy(twiproc.process_name, r->attach_process.process_name, 12);
			AddNote<ELF::Note::twili_process>("Twili", ELF::NT_TWILI_PROCESS, twiproc);
			break; }
		case DEBUG_EVENT_ATTACH_THREAD: {
			printf("    AttachThread:\n");
			printf("      Thread ID: 0x%lx\n", r->attach_thread.thread_id);
			printf("      TLS Pointer: 0x%lx\n", r->attach_thread.tls_pointer);
			printf("      Entrypoint: 0x%lx\n", r->attach_thread.entrypoint);
			AddThread(r->attach_thread.thread_id, r->attach_thread.tls_pointer, r->attach_thread.entrypoint);
			ELF::Note::twili_thread twithread = {
				.thread_id = r->attach_thread.thread_id,
				.tls_pointer = r->attach_thread.tls_pointer,
				.entrypoint = r->attach_thread.entrypoint,
			};
			AddNote<ELF::Note::twili_thread>("Twili", ELF::NT_TWILI_THREAD, twithread);
			break; }
		case DEBUG_EVENT_UNKNOWN:
			printf("    Unknown:\n");
			break;
		case DEBUG_EVENT_EXIT:
			printf("    Exit:\n");
			printf("      Type: 0x%lx\n", r->exit.type);
			break;
		case DEBUG_EVENT_EXCEPTION: {
			printf("    Exception\n");
			const char *exception_types[] = {
				"UNDEFINED_INSTRUCTION",
				"INSTRUCTION_ABORT",
				"DATA_ABORT_MISC",
				"PC_SP_ALIGNMENT_FAULT",
				"DEBUGGER_ATTACHED",
				"BREAKPOINT",
				"USER_BREAK",
				"DEBUGGER_BREAK",
				"BAD_SVC_ID"};
			printf("      Type: 0x%lx [%s]\n", r->exception.exception_type, exception_types[r->exception.exception_type]);
			printf("      Fault Register: 0x%lx\n", r->exception.fault_register);

			int signal;
			switch(r->exception.exception_type) {
			case DEBUG_EXCEPTION_UNDEFINED_INSTRUCTION:
				printf("      Undefined Instruction:\n");
				printf("        Opcode: 0x%x\n", r->exception.undefined_instruction.opcode);
				signal = SIGILL;
				break;
			case DEBUG_EXCEPTION_BREAKPOINT:
				printf("      Breakpoint:\n");
				printf("        Is Watchpoint: 0x%x\n", r->exception.breakpoint.is_watchpoint);
				signal = SIGTRAP;
				break;
			case DEBUG_EXCEPTION_USER_BREAK:
				printf("      User Break:\n");
				printf("        Info: [0x%x, 0x%lx, 0x%lx]\n",
							 r->exception.user_break.info0,
							 r->exception.user_break.info1,
							 r->exception.user_break.info2);
				signal = SIGTRAP;
				break;
			case DEBUG_EXCEPTION_BAD_SVC_ID:
				printf("      Bad SVC ID:\n");
				printf("        SVC ID: 0x%x\n", r->exception.bad_svc_id.svc_id);
				signal = SIGILL;
				break;
			case DEBUG_EXCEPTION_INSTRUCTION_ABORT:
			case DEBUG_EXCEPTION_DATA_ABORT_MISC:
				signal = SIGSEGV;
				break;
			case DEBUG_EXCEPTION_PC_SP_ALIGNMENT_FAULT:
				signal = SIGBUS;
				break;
			case DEBUG_EXCEPTION_DEBUGGER_ATTACHED:
			case DEBUG_EXCEPTION_DEBUGGER_BREAK:
				signal = SIGTRAP;
				break;
			default:
				printf("      Unknown Exception:\n");
				hexdump(&r->exception.bad_svc_id.svc_id, 0x40);
				break;
			}

			if(r->thread_id != 0) {
				printf("Assigning signal %d to thread...\n", signal);
				GetThread(r->thread_id)->signo = signal;
			}
			break; }
		default:
			printf("    Unknown Event:\n");
			hexdump(r->padding, 0x40);
			break;
		}
	} // debug event loop

	// add VMA regions
	uint64_t vaddr = 0;
	do {
		std::tuple<memory_info_t, uint32_t> r = twili::Assert(
			trn::svc::QueryDebugProcessMemory(debug, vaddr));
		memory_info_t mi = std::get<0>(r);

		// skip I/O mappings; these are volatile and reading them might hang or break things
		if(mi.permission & 1 && mi.memory_type != 1) {
			uint32_t elf_flags = 0;
			if(mi.permission & 1) { elf_flags|= ELF::PF_R; }
			if(mi.permission & 2) { elf_flags|= ELF::PF_W; }
			if(mi.permission & 4) { elf_flags|= ELF::PF_X; }
			
			AddVMA((uint64_t) mi.base_addr, mi.size, elf_flags);
		}
		
		vaddr = ((uint64_t) mi.base_addr) + mi.size;
	} while(vaddr > 0);

	for(auto i = threads.begin(); i != threads.end(); i++) {
		AddNote<ELF::Note::elf_prstatus>("CORE", ELF::NT_PRSTATUS, i->second.GeneratePRSTATUS(debug));
	}
	
	size_t total_size = 0;
	total_size+= sizeof(ELF::Elf64_Ehdr);
	for(auto i = vmas.begin(); i != vmas.end(); i++) {
		i->file_offset = total_size;
		total_size+= i->size;
	}
	
	size_t notes_offset = total_size;
	for(auto i = notes.begin(); i != notes.end(); i++) {
		total_size+=
			4 + // namesz
			4 + // descsz
			4 + // type
			i->name.size() +
			i->desc.size();
	}
	
	size_t ph_offset = total_size;
	total_size+= sizeof(ELF::Elf64_Phdr) * (1 + vmas.size());

	bridge::ResponseWriter r = ro.BeginOk(sizeof(uint64_t) + total_size);
	r.Write<uint64_t>(total_size);
	r.Write<ELF::Elf64_Ehdr>({
			.e_ident = {
				.ei_class = ELF::ELFCLASS64,
				.ei_data = ELF::ELFDATALSB,
				.ei_version = 1,
				.ei_osabi = 3 // pretend to be a Linux core dump
			},
			.e_type = ELF::ET_CORE,
			.e_machine = ELF::EM_AARCH64,
			.e_version = 1,
			.e_entry = 0,
			.e_phoff = ph_offset,
			.e_shoff = 0,
			.e_flags = 0,
			.e_phnum = static_cast<uint16_t>(1 + vmas.size()),
			.e_shnum = 0,
			.e_shstrndx = 0,
		});

	// write VMAs
	std::vector<uint8_t> transfer_buffer(r.GetMaxTransferSize(), 0);
	for(auto i = vmas.begin(); i != vmas.end(); i++) {
		for(size_t offset = 0; offset < i->size; offset+= transfer_buffer.size()) {
			size_t size = transfer_buffer.size();
			if(size > i->size - offset) {
				size = i->size - offset;
			}
			twili::Assert(trn::svc::ReadDebugProcessMemory(transfer_buffer.data(), debug, i->virtual_addr + offset, size));
			r.Write(transfer_buffer.data(), size);
		}
	}
			
	// write notes
	std::vector<uint8_t> notes_bytes;
	for(auto i = notes.begin(); i != notes.end(); i++) {
		struct NoteHeader {
			uint32_t namesz;
			uint32_t descsz;
			uint32_t type;
		};
		NoteHeader note_header = {
			.namesz = i->namesz,
			.descsz = i->descsz,
			.type = i->type
		};
		std::vector<uint8_t> note(sizeof(note_header), 0);
		memcpy(note.data(), &note_header, sizeof(note_header)); // this is terrible
		note.insert(note.end(), i->name.begin(), i->name.end());
		note.insert(note.end(), i->desc.begin(), i->desc.end());
		notes_bytes.insert(notes_bytes.end(), note.begin(), note.end());
	}
	r.Write(notes_bytes);
			
	// write phdrs
	std::vector<ELF::Elf64_Phdr> phdrs;
	phdrs.push_back({
			.p_type = ELF::PT_NOTE,
			.p_flags = ELF::PF_R,
			.p_offset = notes_offset,
			.p_vaddr = 0,
			.p_paddr = 0,
			.p_filesz = notes_bytes.size(),
			.p_memsz = 0,
			.p_align = 4
		});
	for(auto i = vmas.begin(); i != vmas.end(); i++) {
		phdrs.push_back({
				.p_type = ELF::PT_LOAD,
				.p_flags = i->flags,
				.p_offset = i->file_offset,
				.p_vaddr = i->virtual_addr,
				.p_paddr = 0,
				.p_filesz = i->size,
				.p_memsz = i->size,
				.p_align = 0x1000
			});
	}
	r.Write(phdrs);
	r.Finalize();
}

ELFCrashReport::Thread::Thread(uint64_t thread_id, uint64_t tls_pointer, uint64_t entrypoint) :
	thread_id(thread_id),
	tls_pointer(tls_pointer),
	entrypoint(entrypoint) {
}

ELF::Note::elf_prstatus ELFCrashReport::Thread::GeneratePRSTATUS(trn::KDebug &debug) {
	ELF::Note::elf_prstatus prstatus = {
		.pr_info = {
			.si_signo = 0,
			.si_code = 0,
			.si_errno = 0,
		},
		.pr_cursig = (int16_t) signo,
		.pr_sigpend = 0,
		.pr_sighold = 0,
		.pr_pid = (uint32_t) thread_id,
		.pr_ppid = 0,
		.pr_pgrp = 0,
		.pr_sid = 0,
		.times = {0},
		.pr_reg = {0}
	};
	thread_context_t context = twili::Assert(trn::svc::GetDebugThreadContext(debug, thread_id, 15)); // where does this 15 number come from?
	memcpy(prstatus.pr_reg, context.regs, sizeof(prstatus.pr_reg));
	return prstatus;
}

}
