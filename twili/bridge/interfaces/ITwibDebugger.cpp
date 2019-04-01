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
#include "../../twili.hpp"
#include "../../process/MonitoredProcess.hpp"

using namespace trn;

namespace twili {
namespace bridge {

ITwibDebugger::ITwibDebugger(uint32_t object_id, Twili &twili, trn::KDebug &&debug, std::shared_ptr<process::MonitoredProcess> proc) : ObjectDispatcherProxy(*this, object_id), twili(twili), debug(std::move(debug)), proc(proc), dispatcher(*this) {
	if(proc) {
		proc->Continue();
	}
}

void ITwibDebugger::QueryMemory(bridge::ResponseOpener opener, uint64_t addr) {
	std::tuple<memory_info_t, uint32_t> info = ResultCode::AssertOk(
		trn::svc::QueryDebugProcessMemory(debug, addr));

	opener.RespondOk(
		std::move(std::get<0>(info)),
		std::move(std::get<1>(info)));
}

void ITwibDebugger::ReadMemory(bridge::ResponseOpener opener, uint64_t addr, uint64_t size) {
	std::vector<uint8_t> buffer(size);
	ResultCode::AssertOk(
		trn::svc::ReadDebugProcessMemory(buffer.data(), debug, addr, buffer.size()));

	opener.RespondOk(std::move(buffer));
}

void ITwibDebugger::WriteMemory(bridge::ResponseOpener opener, uint64_t addr, InputStream &stream) {
	std::shared_ptr<uint64_t> addr_ptr = std::make_shared<uint64_t>(addr);

	printf("starting write memory...\n");
	
	stream.receive =
		[this, addr_ptr](util::Buffer &buf) {
			printf("WriteMemory: stream received 0x%lx bytes\n", buf.ReadAvailable());
			ResultCode::AssertOk(
				trn::svc::WriteDebugProcessMemory(debug, buf.Read(), *addr_ptr, buf.ReadAvailable()));
			*addr_ptr+= buf.ReadAvailable();
			buf.MarkRead(buf.ReadAvailable());
		};

	stream.finish =
		[opener](util::Buffer &buf) {
			printf("WriteMemory: stream finished\n");
			opener.RespondOk();
		};
}

void ITwibDebugger::ListThreads(bridge::ResponseOpener opener) {
	throw ResultError(LIBTRANSISTOR_ERR_UNIMPLEMENTED);
}

void ITwibDebugger::GetDebugEvent(bridge::ResponseOpener opener) {
	debug_event_info_t event = ResultCode::AssertOk(
		trn::svc::GetDebugEvent(debug));

	opener.RespondOk(std::move(event));
}

void ITwibDebugger::GetThreadContext(bridge::ResponseOpener opener, uint64_t thread_id) {
	thread_context_t context = ResultCode::AssertOk(
		trn::svc::GetDebugThreadContext(debug, thread_id, 15));

	opener.RespondOk(std::move(context));
}

void ITwibDebugger::BreakProcess(bridge::ResponseOpener opener) {
	ResultCode::AssertOk(
		trn::svc::BreakDebugProcess(debug));

	opener.RespondOk();
}

void ITwibDebugger::ContinueDebugEvent(bridge::ResponseOpener opener, uint32_t flags, std::vector<uint64_t> thread_ids) {
	// TODO: flags for pre-3.0.0
	ResultCode::AssertOk(
		trn::svc::ContinueDebugEvent(debug, flags, thread_ids.data(), thread_ids.size()));

	opener.RespondOk();
}

void ITwibDebugger::SetThreadContext(bridge::ResponseOpener opener, uint64_t thread_id, uint32_t flags, thread_context_t context) {
	ResultCode::AssertOk(
		trn::svc::SetDebugThreadContext(debug, thread_id, &context, flags));

	opener.RespondOk();
}

void ITwibDebugger::GetNsoInfos(bridge::ResponseOpener opener) {
	uint64_t pid = ResultCode::AssertOk(trn::svc::GetProcessId(debug.handle));
	
	std::vector<service::ldr::NsoInfo> nso_info = ResultCode::AssertOk(
		twili.services.ldr_dmnt.GetNsoInfos(pid));

	opener.RespondOk(std::move(nso_info));
}

void ITwibDebugger::WaitEvent(bridge::ResponseOpener opener) {
	if(wait_handle) {
		throw ResultError(TWILI_ERR_ALREADY_WAITING);
	}
	wait_handle = twili.event_waiter.Add(
		debug,
		[this, opener]() mutable -> bool {
			printf("sending response\n");
			opener.RespondOk();
			printf("sent response, resetting wait handle\n");
			wait_handle.reset();
			printf("reset wait handle\n");
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
				ResultCode::AssertOk(
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
			printf("found %d modules, skipping first (rtld) module and picking second (main)\n", candidates.size());
			addr = candidates[1]; // main
		}
	}
	
	opener.RespondOk(std::move(addr));
}

void ITwibDebugger::LaunchDebugProcess(bridge::ResponseOpener opener) {
	uint64_t pid = ResultCode::AssertOk(trn::svc::GetProcessId(debug.handle));

	if(env_get_kernel_version() >= KERNEL_VERSION_500) {
		ResultCode::AssertOk(
			twili.services.pm_dmnt.SendSyncRequest<1>( // LaunchDebugProcess
				ipc::InRaw<uint64_t>(pid)));
	} else {
		ResultCode::AssertOk(
			twili.services.pm_dmnt.SendSyncRequest<2>( // LaunchDebugProcess
				ipc::InRaw<uint64_t>(pid)));
	}
	
	opener.RespondOk();
}

} // namespace bridge
} // namespace twili
