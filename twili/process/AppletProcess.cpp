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

#include "AppletProcess.hpp"

#include "../twili.hpp"
#include "fs/ActualFile.hpp"
#include "fs/VectorFile.hpp"

#include "err.hpp"
#include "applet_shim.hpp"

using namespace trn;

namespace twili {
namespace process {

AppletProcess::AppletProcess(Twili &twili) :
	MonitoredProcess(twili),
	command_wevent(command_event) {

	// build virtual exefs
	virtual_exefs.SetRtld(std::make_shared<fs::ActualFile>(fopen("/squash/applet_host.nso", "rb")));
	virtual_exefs.SetNpdm(std::make_shared<fs::ActualFile>(fopen("/squash/applet_host.npdm", "rb")));
}

void AppletProcess::Launch(bridge::ResponseOpener opener) {
	run_opener = opener;
	ChangeState(State::Started);
	
	// this pointer cast is not cool.
	twili.applet_tracker.QueueLaunch(std::dynamic_pointer_cast<AppletProcess>(shared_from_this()));
}

void AppletProcess::ChangeState(State state) {
	MonitoredProcess::ChangeState(state);
	if(state == State::Attached) {
		ecs_pending = false;
		
		if(run_opener) {
			auto w = run_opener->BeginOk(sizeof(uint64_t));
			w.Write<uint64_t>(GetPid());
			w.Finalize();
			run_opener.reset();
		}

		// find target entry...
		memory_info_t mi = std::get<0>(ResultCode::AssertOk(svc::QueryProcessMemory(*proc, 0)));
		int coderegions_found = 0;
		while((uint64_t) mi.base_addr + mi.size > 0) {
			if(mi.memory_type == 3 && mi.permission == 5) {
				coderegions_found++;
				if(coderegions_found == 2) { // first region is AppletShim, second region is target
					printf("found target entry at 0x%lx\n", (uint64_t) mi.base_addr);
					target_entry = (uint64_t) mi.base_addr;
				}
			}
			mi = std::get<0>(ResultCode::AssertOk(svc::QueryProcessMemory(*proc, (uint64_t) mi.base_addr + mi.size)));
		}
		if(coderegions_found != 2) {
			printf("CODE REGION COUNT MISMATCH (expected 2, counted %d)\n", coderegions_found);
		}
	}
	if(state == State::Exited) {
		if(run_opener) {
			run_opener->BeginError(GetResult()).Finalize();
			run_opener.reset();
		}
		if(ecs_pending) {
			ResultCode::AssertOk(
				twili.services.ldr_shel.SendSyncRequest<65001>( // ClearExternalContentSource
					trn::ipc::InRaw<uint64_t>(applet_shim::TitleId)));
		}
		kill_timeout.reset();
	}
}

void AppletProcess::Kill() {
	if(GetState() == State::Created) {
		ChangeState(State::Exited);
		return;
	}

	if(GetState() == State::Started) {
		// we're already been added to AppletTracker queue...
		// just set this flag so AppletTracker can skip over us
		// when we reach the front of the queue.
		exit_pending = true;
		return;
	}
	
	if(GetState() == State::Exited) {
		return; // nothing to do here
	}
	
	std::shared_ptr<MonitoredProcess> self = shared_from_this();

	// if we don't exit within 10 seconds, go ahead and terminate it
	kill_timeout = twili.event_waiter.AddDeadline(
		svcGetSystemTick() + 10000000000uLL,
		[self, this]() -> uint64_t {
			printf("AppletProcess: clean exit timeout expired, terminating...\n");
			self->Terminate();
			kill_timeout.reset(); // make sure self ptr gets cleaned up
			return 0; // don't rearm
		});

	printf("AppletProcess: requesting to exit cleanly...\n");
	PushCommand(0); // REQUEST_EXIT
}

void AppletProcess::AddHBABIEntries(std::vector<loader_config_entry_t> &entries) {
	MonitoredProcess::AddHBABIEntries(entries);
	entries.push_back({
			.key = LCONFIG_KEY_APPLET_TYPE,
			.flags = 0,
			.applet_type = {
				.applet_type = LCONFIG_APPLET_TYPE_LIBRARY_APPLET,
			}
		});
}

bool AppletProcess::PrepareForLaunch() {
	if(exit_pending) {
		ChangeState(State::Exited);
		return false;
	}
	
	if(files.size() > 1) {
		throw ResultError(TWILI_ERR_TOO_MANY_MODULES);
	} else if(files.empty()) {
		throw ResultError(TWILI_ERR_NO_MODULES);
	}
	virtual_exefs.SetMain(fs::NRONSOTransmutationFile::Create(files.front()));

	printf("installing ExternalContentSource\n");
	KObject session;
	ResultCode::AssertOk(
		twili.services.ldr_shel.SendSyncRequest<65000>( // SetExternalContentSource
			trn::ipc::InRaw<uint64_t>(applet_shim::TitleId),
			trn::ipc::OutHandle<KObject, trn::ipc::move>(session)));
	printf("installed ExternalContentSource\n");
	ecs_pending = true;
	
	ResultCode::AssertOk(
		twili.server.AttachSession<fs::ProcessFileSystem::IFileSystem>(std::move(session), virtual_exefs));

	return true;
}

trn::KEvent &AppletProcess::GetCommandEvent() {
	return command_event;
}

std::optional<uint32_t> AppletProcess::PopCommand() {
	if(commands.size() == 0) {
		return std::nullopt;
	} else {
		uint32_t cmd = commands.front();
		commands.pop_front();
		return cmd;
	}
}

void AppletProcess::PushCommand(uint32_t command) {
	commands.push_back(command);
	command_wevent.Signal();
}

} // namespace process
} // namespace twili
