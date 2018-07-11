#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/cpp/waiter.hpp>
#include<libtransistor/util.h>
#include<libtransistor/svc.h>
#include<libtransistor/ipc/twili.h>
#include<libtransistor/ipc/fatal.h>
#include<libtransistor/runtime_config.h>
#include<libtransistor/usb_serial.h>

#include<unistd.h>
#include<stdio.h>
#include<errno.h>

#include "process_creation.hpp"
#include "util.hpp"

uint8_t _local_heap[16 * 1024 * 1024]; // 16 MiB

runconf_stdio_override_t _trn_runconf_stdio_override = _TRN_RUNCONF_STDIO_OVERRIDE_USB_SERIAL;
runconf_heap_mode_t _trn_runconf_heap_mode = _TRN_RUNCONF_HEAP_MODE_OVERRIDE;

void *_trn_runconf_heap_base = _local_heap;
size_t _trn_runconf_heap_size = sizeof(_local_heap);

void ReportCrash(uint64_t pid);

int main(int argc, char *argv[]) {
	printf("===TWILI LAUNCHER===\n");

	if(auto twili_result = twili::util::ReadFile("/squash/twili.nro")) {
		auto twili_image = *twili_result;
		printf("Read Twili (0x%lx bytes)\n", twili_image.size());

		try {
			std::vector<uint32_t> caps = {
				0b00011111111111111111111111101111, // SVC grants
				0b00111111111111111111111111101111,
				0b01011111111111111111111111101111,
				0b01100000000000001111111111101111,
				0b10011111111111111111111111101111,
				0b10100000000000000000111111101111,
				0b00000010000000000111001110110111, // KernelFlags
				0b00000000000000000101111111111111, // ApplicationType
				0b00000000000110000011111111111111, // KernelReleaseVersion
				0b00000010000000000111111111111111, // HandleTableSize
				0b00000000000001001111111111111111, // DebugFlags (can debug others)
			};

			twili::process_creation::ProcessBuilder builder("twili", caps);
			FILE *twili = fopen("/squash/twili.nro", "rb");
			twili::process_creation::ProcessBuilder::FileDataReader data_reader(twili);
			trn::ResultCode::AssertOk(builder.AppendNRO(data_reader));
			auto proc = trn::ResultCode::AssertOk(builder.Build());
			fclose(twili);
			
			// launch Twili
			printf("Launching...\n");
			trn::ResultCode::AssertOk(
				trn::svc::StartProcess(*proc, 58, 0, 0x100000));

			trn::Waiter waiter;
			auto wh = waiter.Add(*proc, [&proc]() {
					proc->ResetSignal();
					auto state = (trn::svc::ProcessState) trn::ResultCode::AssertOk(trn::svc::GetProcessInfo(*proc, 0));
					if(state == trn::svc::ProcessState::CRASHED) {
						trn::ResultCode::AssertOk(usb_serial_init());
						
						// set up serial console
						int usb_fd = usb_serial_open_fd();
						if(usb_fd < 0) {
							throw trn::ResultError(-usb_fd);
						}
						dup2(usb_fd, STDOUT_FILENO);
						dup2(usb_fd, STDERR_FILENO);
						dup2(usb_fd, STDIN_FILENO);
						dbg_set_file(fd_file_get(usb_fd));
						printf("twili has crashed\n");
						uint64_t pid = trn::ResultCode::AssertOk(
							trn::svc::GetProcessId(proc->handle));
						printf("  pid 0x%lx\n", pid);
						ReportCrash(pid);
						trn::svc::TerminateProcess(*proc);
						return false;
					}
					return true;
				});
			
			
			printf("Launched\n");
			//while(1) {
			//	waiter.Wait(3000000000);
			//}
		} catch(trn::ResultError e) {
			printf("caught ResultError: %s\n", e.what());
			fatal_init();
			fatal_transition_to_fatal_error(e.code.code, 0);
		}
	} else {
		return 1;
	}
	
	return 0;
}

const char *register_labels[33] = {
	" x0", " x1", " x2", " x3",
	" x4", " x5", " x6", " x7",
	" x8", " x9", "x10", "x11",
	"x12", "x13", "x14", "x15",
	"x16", "x17", "x18", "x19",
	"x20", "x21", "x22", "x23",
	"x24", "x25", "x26", "x27",
	"x28", "x29", "x30", " sp",
	" pc" };

void ReportCrash(uint64_t pid) {
	printf("generating crash report...\n");
	trn::KDebug debug = trn::ResultCode::AssertOk(
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
			break; }
		case DEBUG_EVENT_ATTACH_THREAD: {
			printf("    AttachThread:\n");
			printf("      Thread ID: 0x%lx\n", r->attach_thread.thread_id);
			printf("      TLS Pointer: 0x%lx\n", r->attach_thread.tls_pointer);
			printf("      Entrypoint: 0x%lx\n", r->attach_thread.entrypoint);
			thread_context_t context = trn::ResultCode::AssertOk(trn::svc::GetDebugThreadContext(debug, r->attach_thread.thread_id, 15));
			for(uint32_t i = 0; i < ARRAY_LENGTH(register_labels); i++) {
				printf("%s: 0x%016lx, ", register_labels[i], context.regs[i]);
				if(i%2 == 1) {
					printf("\n");
				}
			}
			printf("\n");
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
			case DEBUG_EXCEPTION_INSTRUCTION_ABORT:
			case DEBUG_EXCEPTION_DATA_ABORT_MISC:
			case DEBUG_EXCEPTION_PC_SP_ALIGNMENT_FAULT:
			case DEBUG_EXCEPTION_DEBUGGER_ATTACHED:
			case DEBUG_EXCEPTION_DEBUGGER_BREAK:
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
	printf("Memory Map:\n");
	uint64_t vaddr = 0;
	do {
		std::tuple<memory_info_t, uint32_t> r = trn::ResultCode::AssertOk(
			trn::svc::QueryDebugProcessMemory(debug, vaddr));
		memory_info_t mi = std::get<0>(r);

		char perm[4] = {'-', '-', '-', 0};
		if(mi.permission & 1) { perm[0] = 'R'; }
		if(mi.permission & 2) { perm[1] = 'W'; }
		if(mi.permission & 4) { perm[2] = 'X'; }

		printf("  0x%016lx - 0x%016lx    %s    0x%x    0x%x\n", (uint64_t) mi.base_addr, (uint64_t) mi.base_addr + mi.size, perm, mi.memory_type, mi.memory_attribute);
		vaddr+= mi.size;
	} while(vaddr > 0);
}
