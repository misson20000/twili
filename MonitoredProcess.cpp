#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/util.h>

#include "MonitoredProcess.hpp"
#include "ELFCrashReport.hpp"
#include "twili.hpp"

namespace twili {

using Transistor::ResultCode;

MonitoredProcess::MonitoredProcess(Twili *twili, std::shared_ptr<Transistor::KProcess> proc) :
	twili(twili),
	proc(proc) {

	printf("created monitored process: 0x%x\n", proc->handle);
	wait = twili->event_waiter.Add(*proc, [this]() {
			printf("monitored process (0x%x) signalled\n", this->proc->handle);
			this->proc->ResetSignal();
			auto state = (Transistor::SVC::ProcessState) ResultCode::AssertOk(Transistor::SVC::GetProcessInfo(*this->proc, 0));
			printf("  state: 0x%lx\n", state);
			if(state == Transistor::SVC::ProcessState::CRASHED) {
				printf("monitored process (0x%x) crashed\n", this->proc->handle);
				printf("ready to generate crash report\n");
				crashed = true;
			}
			if(state == Transistor::SVC::ProcessState::EXITED) {
				printf("monitored process (0x%x) exited\n", this->proc->handle);
				destroy_flag = true;
			}
			return true;
		});
}

void MonitoredProcess::Launch() {
	printf("launching monitored process: 0x%x\n", proc->handle);
	Transistor::ResultCode::AssertOk(
		Transistor::SVC::StartProcess(*proc, 58, 0, 0x100000));
}

class CrashReportException {
 public:
	CrashReportException(debug_event_info_t &info) :
		info(info) {
	}

	debug_event_info_t info;
};

class CrashReportThread {
 public:
	CrashReportThread(debug_event_info_t &info) :
		thread_id(info.attach_thread.thread_id),
		tls_pointer(info.attach_thread.tls_pointer),
		entrypoint(info.attach_thread.entrypoint) {
		
	}

	uint64_t thread_id;
	uint64_t tls_pointer;
	uint64_t entrypoint;
};

Transistor::Result<std::nullopt_t> MonitoredProcess::CoreDump(usb::USBBridge::USBResponseWriter &writer) {
	ELFCrashReport report;
	
	printf("generating crash report...\n");
	
	Transistor::KDebug debug = ResultCode::AssertOk(
		Transistor::SVC::DebugActiveProcess(
			ResultCode::AssertOk(
				Transistor::SVC::GetProcessId(proc->handle))));
	printf("  opened debug: 0x%x\n", debug.handle);
	while(1) {
		auto r = Transistor::SVC::GetDebugEvent(debug);
		if(!r) {
			if(r.error().code == 0x8c01) {
				break;
			} else {
				throw new Transistor::ResultError(r.error());
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
			break; }
		case DEBUG_EVENT_ATTACH_THREAD: {
			printf("    AttachThread:\n");
			printf("      Thread ID: 0x%lx\n", r->attach_thread.thread_id);
			printf("      TLS Pointer: 0x%lx\n", r->attach_thread.tls_pointer);
			printf("      Entrypoint: 0x%lx\n", r->attach_thread.entrypoint);
			ELF::Note::elf_prstatus prstatus = {
				.pr_info = {
					.si_signo = 0,
					.si_code = 0,
					.si_errno = 0,
				},
				.pr_cursig = 0,
				.pr_sigpend = 0,
				.pr_sighold = 0,
				.pr_pid = (uint32_t) r->attach_thread.thread_id,
				.pr_ppid = 0,
				.pr_pgrp = 0,
				.pr_sid = 0,
				.times = {0},
				.pr_reg = {0}
			};
			report.AddNote<ELF::Note::elf_prstatus>("CORE", ELF::NT_PRSTATUS, prstatus);
			
			// FPREGSET
			// X86_XSTATE
			// SIGINFO
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
			switch(r->exception.exception_type) {
			case DEBUG_EXCEPTION_UNDEFINED_INSTRUCTION:
				printf("      Undefined Instruction:\n");
				printf("        Opcode: 0x%x\n", r->exception.undefined_instruction.opcode);
				break;
			case DEBUG_EXCEPTION_BREAKPOINT:
				printf("      Breakpoint:\n");
				printf("        Is Watchpoint: 0x%x\n", r->exception.breakpoint.is_watchpoint);
				break;
			case DEBUG_EXCEPTION_USER_BREAK:
				printf("      User Break:\n");
				printf("        Info: [0x%x, 0x%lx, 0x%lx]\n",
							 r->exception.user_break.info0,
							 r->exception.user_break.info1,
							 r->exception.user_break.info2);
				break;
			case DEBUG_EXCEPTION_BAD_SVC_ID:
				printf("      Bad SVC ID:\n");
				printf("        SVC ID: 0x%x\n", r->exception.bad_svc_id.svc_id);
				break;
			default:
				printf("      Unknown Exception:\n");
				hexdump(&r->exception.bad_svc_id.svc_id, 0x40);
				break;
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
			Transistor::SVC::QueryDebugProcessMemory(debug, vaddr));
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

	auto r = report.Generate(debug, writer);
	Transistor::SVC::TerminateProcess(*this->proc);
	return r;
}

MonitoredProcess::~MonitoredProcess() {
}

}
