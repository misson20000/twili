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

#include "ITwibDebugger.hpp"

#include<libtransistor/cpp/svc.hpp>

#include "err.hpp"
#include "title_id.hpp"
#include "../../twili.hpp"
#include "../../Services.hpp"
#include "../../process/MonitoredProcess.hpp"

using namespace trn;

namespace twili {
namespace bridge {

ITwibDebugger::ITwibDebugger(uint32_t object_id, Twili &twili, trn::KDebug &&debug, std::shared_ptr<process::MonitoredProcess> proc) : ObjectDispatcherProxy(*this, object_id), twili(twili), debug(std::move(debug)), proc(proc), dispatcher(*this) {
	if(proc) {
		proc->Continue();
	}
	
	PumpEvents();

	if(title_id != 0) {
		auto r = twili.debugging_titles.insert(title_id);
		if(!r.second) {
			twili::Abort(TWILI_ERR_INVALID_DEBUGGER_STATE);
		}
	} else {
		twili::Abort(TWILI_ERR_INVALID_DEBUGGER_STATE);
	}
}

ITwibDebugger::~ITwibDebugger() {
	if(title_id != 0) {
		twili.debugging_titles.erase(title_id);
	}
}

void ITwibDebugger::PumpEvents() {
	auto r = trn::svc::GetDebugEvent(debug);
	while(r) {
		debug_event_info_t e = *r;
		event_queue.push_back(e);

		if(e.event_type == DEBUG_EVENT_ATTACH_PROCESS) {
			if(title_id != 0) {
				// kernel is doing something weird to us
				twili::Abort(TWILI_ERR_INVALID_DEBUGGER_STATE);
			}
			title_id = e.attach_process.title_id;
		}
		
		r = trn::svc::GetDebugEvent(debug);
	}
	
	if(r.error().code != 0x8c01) {
		twili::Abort(r.error());
	}
}

void ITwibDebugger::QueryMemory(bridge::ResponseOpener opener, uint64_t addr) {
	auto r = trn::svc::QueryDebugProcessMemory(debug, addr);
	if(!r) {
		TWILI_BRIDGE_CHECK(r.error());
	}
	
	std::tuple<memory_info_t, uint32_t> info = *r;

	opener.RespondOk(
		std::move(std::get<0>(info)),
		std::move(std::get<1>(info)));
}

void ITwibDebugger::ReadMemory(bridge::ResponseOpener opener, uint64_t addr, uint64_t size) {
	std::vector<uint8_t> buffer(size);
	
	TWILI_BRIDGE_CHECK(twili::Unwrap(trn::svc::ReadDebugProcessMemory(buffer.data(), debug, addr, buffer.size())));

	opener.RespondOk(std::move(buffer));
}

void ITwibDebugger::WriteMemory(bridge::ResponseOpener opener, uint64_t addr, InputStream &stream) {
	std::shared_ptr<uint64_t> addr_ptr = std::make_shared<uint64_t>(addr);
	std::shared_ptr<trn::ResultCode> r = std::make_shared<trn::ResultCode>(RESULT_OK);
	
	printf("starting write memory...\n");
	
	stream.receive =
		[this, addr_ptr, r](util::Buffer &buf) {
			if(*r == RESULT_OK) {
				printf("WriteMemory: stream received 0x%lx bytes\n", buf.ReadAvailable());
				// TODO: what happens if we keep streaming data in?
				*r = twili::Unwrap(trn::svc::WriteDebugProcessMemory(debug, buf.Read(), *addr_ptr, buf.ReadAvailable()));
				*addr_ptr+= buf.ReadAvailable();
				buf.MarkRead(buf.ReadAvailable());
			} else {
				printf("WriteMemory: discarding due to error 0x%x\n", r->code);
				buf.MarkRead(buf.ReadAvailable());
			}
		};

	stream.finish =
		[opener, r](util::Buffer &buf) {
			printf("WriteMemory: stream finished\n");
			TWILI_BRIDGE_CHECK(*r);
			opener.RespondOk();
		};
}

void ITwibDebugger::ListThreads(bridge::ResponseOpener opener) {
	opener.RespondError(LIBTRANSISTOR_ERR_UNIMPLEMENTED);
}

void ITwibDebugger::GetDebugEvent(bridge::ResponseOpener opener) {
	PumpEvents();
	
	if(event_queue.empty()) {
		opener.RespondError(0x8c01);
	} else {
		opener.RespondOk(std::move(event_queue.front()));
		event_queue.pop_front();
	}
}

void ITwibDebugger::GetThreadContext(bridge::ResponseOpener opener, uint64_t thread_id) {
	auto r = trn::svc::GetDebugThreadContext(debug, thread_id, 15);
	if(!r) {
		TWILI_BRIDGE_CHECK(r.error());
	}

	opener.RespondOk(std::move(*r));
}

void ITwibDebugger::BreakProcess(bridge::ResponseOpener opener) {
	TWILI_BRIDGE_CHECK(twili::Unwrap(trn::svc::BreakDebugProcess(debug)));

	opener.RespondOk();
}

void ITwibDebugger::ContinueDebugEvent(bridge::ResponseOpener opener, uint32_t flags, std::vector<uint64_t> thread_ids) {
	// TODO: flags for pre-3.0.0
	TWILI_BRIDGE_CHECK(twili::Unwrap(trn::svc::ContinueDebugEvent(debug, flags, thread_ids.data(), thread_ids.size())));

	opener.RespondOk();
}

void ITwibDebugger::SetThreadContext(bridge::ResponseOpener opener, uint64_t thread_id, uint32_t flags, thread_context_t context) {
	auto r = trn::svc::SetDebugThreadContext(debug, thread_id, &context, flags);
	if(r) {
		opener.RespondOk();
	} else {
		opener.RespondError(r.error());
	}
}

void ITwibDebugger::GetNsoInfos(bridge::ResponseOpener opener) {
	TWILI_BRIDGE_CHECK(
		twili.debugging_titles.find(title_id::LoaderTitle) != twili.debugging_titles.end() ?
		TWILI_ERR_SYSMODULE_BEING_DEBUGGED :
		RESULT_OK);
	
	uint64_t pid = twili::Assert(trn::svc::GetProcessId(debug.handle));
	
	std::vector<hos_types::LoadedModuleInfo> nso_info;
	TWILI_BRIDGE_CHECK(twili.services->GetNsoInfos(pid, &nso_info));

	opener.RespondOk(std::move(nso_info));
}

void ITwibDebugger::WaitEvent(bridge::ResponseOpener opener) {
	TWILI_BRIDGE_CHECK(
		wait_handle ? TWILI_ERR_ALREADY_WAITING : RESULT_OK);

	wait_handle = twili.event_waiter.Add(
		debug,
		[this, opener]() mutable -> bool {
			opener.RespondOk();
			wait_handle.reset();
			return false;
		});
}

void ITwibDebugger::GetTargetEntry(bridge::ResponseOpener opener) {
	uint64_t addr = 0;

	if(proc) {
		addr = proc->GetTargetEntry();
	} else {
		std::vector<uint64_t> candidates;
		memory_info_t mi;
		do {
			mi = std::get<0>(
				twili::Assert(
					trn::svc::QueryDebugProcessMemory(debug, addr)));

			if(mi.memory_type == 3 && mi.permission == 5) { // CODE_STATIC RX
				printf("found module base at 0x%lx\n", addr);
				candidates.push_back((uint64_t) mi.base_addr);
			}
			addr = (uint64_t) mi.base_addr + mi.size;
		} while(addr > (uint64_t) mi.base_addr);

		if(candidates.size() == 0) {
			printf("no modules found\n");
			addr = 0;
		} else if(candidates.size() == 1) { // only main
			printf("only found one module, assuming main\n");
			addr = candidates[0];
		} else { // rtld, main, subsdk[0-9], sdk
			printf("found %zd modules, skipping first (rtld) module and picking second (main)\n", candidates.size());
			addr = candidates[1]; // main
		}
	}
	
	opener.RespondOk(std::move(addr));
}

void ITwibDebugger::LaunchDebugProcess(bridge::ResponseOpener opener) {
	uint64_t pid = twili::Assert(trn::svc::GetProcessId(debug.handle));

	TWILI_BRIDGE_CHECK(twili.services->StartProcess(pid));
	
	opener.RespondOk();
}

void ITwibDebugger::GetNroInfos(bridge::ResponseOpener opener) {
	TWILI_BRIDGE_CHECK( // TODO: only need to check before 3.0.0
		twili.debugging_titles.find(title_id::LoaderTitle) != twili.debugging_titles.end() ?
		TWILI_ERR_SYSMODULE_BEING_DEBUGGED :
		RESULT_OK);
	TWILI_BRIDGE_CHECK(
		twili.debugging_titles.find(title_id::RoTitle) != twili.debugging_titles.end() ?
		TWILI_ERR_SYSMODULE_BEING_DEBUGGED :
		RESULT_OK);

	uint64_t pid = twili::Assert(trn::svc::GetProcessId(debug.handle));

	std::vector<hos_types::LoadedModuleInfo> nro_info;
	TWILI_BRIDGE_CHECK(twili.services->GetNroInfos(pid, &nro_info));
	opener.RespondOk(std::move(nro_info));
}

} // namespace bridge
} // namespace twili
