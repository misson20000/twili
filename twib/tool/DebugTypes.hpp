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

namespace twili {
namespace twib {
namespace nx {

using PageInfo = uint32_t;

struct MemoryInfo {
	uint64_t base_addr;
	uint64_t size;
	uint32_t memory_type;
	uint32_t memory_attribute;
	uint32_t permission;
	uint32_t device_ref_count;
	uint32_t ipc_ref_count;
	uint32_t padding;
};

struct DebugEvent {
	enum class EventType : uint32_t {
		AttachProcess = 0,
		AttachThread = 1,
		ExitProcess = 2,
		ExitThread = 3,
		Exception = 4,
	};
	enum class ExitType : uint64_t {
		PausedThread = 0,
		RunningThread = 1,
		ExitedProcess = 2,
		TerminatedProcess = 3,
	};
	enum class ExceptionType : uint32_t {
		Trap = 0,
		InstructionAbort = 1,
		DataAbortMisc = 2,
		PcSpAlignmentFault = 3,
		DebuggerAttached = 4,
		BreakPoint = 5,
		UserBreak = 6,
		DebuggerBreak = 7,
		BadSvcId = 8,
		SError = 9,
	};

	EventType event_type;
	uint32_t flags;
	uint64_t thread_id;
	union {
		struct {
			uint64_t title_id;
			uint64_t process_id;
			char process_name[12];
			uint32_t mmu_flags;
			uint64_t user_exception_context_addr; // [5.0.0+]
		} attach_process;
		struct {
			uint64_t thread_id;
			uint64_t tls_pointer;
			uint64_t entrypoint;
		} attach_thread;
		struct {
			ExitType type;
		} exit;
		struct {
			ExceptionType exception_type;
			uint64_t fault_register;
			union {
				struct {
					uint32_t opcode;
				} undefined_instruction;
				struct {
					uint32_t is_watchpoint;
				} breakpoint;
				struct {
					uint32_t info0;
					uint64_t info1;
					uint64_t info2;
				} user_break;
				struct {
					uint32_t svc_id;
				} bad_svc_id;
			};
		} exception;
		uint8_t padding[0x80]; // not sure how large this actually needs to be, but let's be safe.
	};
};

struct ThreadContext {
	union {
		uint64_t regs[100];
		struct {
			uint64_t x[31];
			uint64_t sp, pc;
		};
	};
};

struct LoadedModuleInfo {
	uint8_t build_id[0x20];
	uint64_t base_addr;
	uint64_t size;
};

} // namespace nx
} // namespace tiwb
} // namespace twili
