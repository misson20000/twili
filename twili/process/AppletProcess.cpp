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
	virtual_exefs.SetRtld(std::make_shared<fs::ActualFile>(fopen("/squash/twili_applet_shim.nso", "rb")));
	virtual_exefs.SetNpdm(std::make_shared<fs::ActualFile>(fopen("/squash/default.npdm", "rb")));
}

void AppletProcess::AppendCode(std::vector<uint8_t> nso) {
	if(has_code) {
		throw ResultError(TWILI_ERR_ALREADY_HAS_CODE);
	} else {
		virtual_exefs.SetMain(std::make_shared<fs::VectorFile>(nso));
		has_code = true;
	}
}

void AppletProcess::Launch(bridge::ResponseOpener opener) {
	run_opener = opener;
	ChangeState(State::Started);
	
	// this pointer cast is not cool.
	twili.applet_tracker.QueueLaunch(std::dynamic_pointer_cast<AppletProcess>(shared_from_this()));
}

void AppletProcess::ChangeState(State state) {
	MonitoredProcess::ChangeState(state);
	if(state == State::Attached && run_opener) {
		auto w = run_opener->BeginOk(sizeof(uint64_t));
		w.Write<uint64_t>(GetPid());
		w.Finalize();
		run_opener.reset();

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
	if(state == State::Exited && run_opener) {
		run_opener->BeginError(GetResult()).Finalize();
		run_opener.reset();
	}
	if(state == State::Exited) {
		kill_timeout.reset();
	}
}

void AppletProcess::Kill() {
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

void AppletProcess::PrepareForLaunch() {
	printf("installing ExternalContentSource\n");
	KObject session;
	ResultCode::AssertOk(
		twili.services.ldr_shel.SendSyncRequest<65000>( // SetExternalContentSource
			trn::ipc::InRaw<uint64_t>(applet_shim::TitleId),
			trn::ipc::OutHandle<KObject, trn::ipc::move>(session)));
	printf("installed ExternalContentSource\n");
	ResultCode::AssertOk(
		twili.server.AttachSession<fs::ProcessFileSystem::IFileSystem>(std::move(session), virtual_exefs));
}

trn::KEvent &AppletProcess::GetCommandEvent() {
	return command_event;
}

uint32_t AppletProcess::PopCommand() {
	if(commands.size() == 0) {
		throw trn::ResultError(TWILI_ERR_APPLET_SHIM_NO_COMMANDS_LEFT);
	}
	uint32_t cmd = commands.front();
	commands.pop_front();
	return cmd;
}

void AppletProcess::PushCommand(uint32_t command) {
	commands.push_back(command);
	command_wevent.Signal();
}

} // namespace process
} // namespace twili
