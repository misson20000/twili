#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/util.h>

#include "ELFCrashReport.hpp"

#include<vector>
#include<string>

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

void ELFCrashReport::Generate(trn::KDebug &debug, twili::bridge::ResponseOpener ro) {
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

	bridge::ResponseWriter r = ro.BeginOk(total_size);
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
			ResultCode::AssertOk(trn::svc::ReadDebugProcessMemory(transfer_buffer.data(), debug, i->virtual_addr + offset, size));
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
	thread_context_t context = ResultCode::AssertOk(trn::svc::GetDebugThreadContext(debug, thread_id, 15)); // where does this 15 number come from?
	memcpy(prstatus.pr_reg, context.regs, sizeof(prstatus.pr_reg));
	return prstatus;
}

}
