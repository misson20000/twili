#include "Process.hpp"

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/cpp/ipcclient.hpp>
#include<libtransistor/cpp/ipc/sm.hpp>
#include<libtransistor/util.h>

using trn::ResultCode;

namespace twili {

Process::Process(uint64_t pid) : pid(pid) {
}

struct NsoInfo {
	uint64_t addr;
	size_t size;
	union {
		uint8_t build_id[0x20];
		uint64_t build_id_64[4];
	};
};

trn::Result<std::nullopt_t> Process::GenerateCrashReport(ELFCrashReport &report, usb::USBBridge::USBResponseOpener opener) {
	// write nso info notes
	{
		trn::service::SM sm = ResultCode::AssertOk(trn::service::SM::Initialize());
		trn::ipc::client::Object ldr_dmnt = ResultCode::AssertOk(
			sm.GetService("ldr:dmnt"));
		std::vector<NsoInfo> nso_info(16, {0, 0, 0});
		uint32_t num_nso_infos;
		auto r = ldr_dmnt.SendSyncRequest<2>( // GetNsoInfos
			trn::ipc::InRaw<uint64_t>(pid),
			trn::ipc::OutRaw<uint32_t>(num_nso_infos),
			trn::ipc::Buffer<NsoInfo, 0xA>(nso_info.data(), nso_info.size() * sizeof(NsoInfo)));
		if(r) {
			for(uint32_t i = 0; i < num_nso_infos; i++) {
				ELF::Note::twili_nso_info twinso = {
					.addr = nso_info[i].addr,
					.size = nso_info[i].size,
				};
				memcpy(twinso.build_id, nso_info[i].build_id, sizeof(nso_info[i].build_id));
				report.AddNote<ELF::Note::twili_nso_info>("Twili", ELF::NT_TWILI_NSO, twinso);
			}
		}
	}
	
	trn::KDebug debug = ResultCode::AssertOk(
		trn::svc::DebugActiveProcess(pid));
	printf("  opened debug: 0x%x\n", debug.handle);

	while(1) {
		auto r = trn::svc::GetDebugEvent(debug);
		if(!r) {
			if(r.error().code == 0x8c01) {
				break;
			} else {
				throw new trn::ResultError(r.error());
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
			report.AddNote<ELF::Note::elf_prpsinfo>("CORE", ELF::NT_PRPSINFO, psinfo);
			ELF::Note::twili_process twiproc = {
				.title_id = r->attach_process.title_id,
				.process_id = r->attach_process.process_id,
				.mmu_flags = r->attach_process.mmu_flags,
			};
			memcpy(twiproc.process_name, r->attach_process.process_name, 12);
			report.AddNote<ELF::Note::twili_process>("Twili", ELF::NT_TWILI_PROCESS, twiproc);
			break; }
		case DEBUG_EVENT_ATTACH_THREAD: {
			printf("    AttachThread:\n");
			printf("      Thread ID: 0x%lx\n", r->attach_thread.thread_id);
			printf("      TLS Pointer: 0x%lx\n", r->attach_thread.tls_pointer);
			printf("      Entrypoint: 0x%lx\n", r->attach_thread.entrypoint);
			report.AddThread(r->attach_thread.thread_id, r->attach_thread.tls_pointer, r->attach_thread.entrypoint);
			ELF::Note::twili_thread twithread = {
				.thread_id = r->attach_thread.thread_id,
				.tls_pointer = r->attach_thread.tls_pointer,
				.entrypoint = r->attach_thread.entrypoint,
			};
			report.AddNote<ELF::Note::twili_thread>("Twili", ELF::NT_TWILI_THREAD, twithread);
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
				report.GetThread(r->thread_id)->signo = signal;
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
		std::tuple<memory_info_t, uint32_t> r = ResultCode::AssertOk(
			trn::svc::QueryDebugProcessMemory(debug, vaddr));
		memory_info_t mi = std::get<0>(r);

		if(mi.permission & 1) {
			uint32_t elf_flags = 0;
			if(mi.permission & 1) { elf_flags|= ELF::PF_R; }
			if(mi.permission & 2) { elf_flags|= ELF::PF_W; }
			if(mi.permission & 4) { elf_flags|= ELF::PF_X; }
			
			report.AddVMA((uint64_t) mi.base_addr, mi.size, elf_flags);
		}
		
		vaddr = ((uint64_t) mi.base_addr) + mi.size;
	} while(vaddr > 0);

	return report.Generate(debug, opener);
}

} // namespace twili
