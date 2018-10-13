#include "AppletProcess.hpp"

#include "../twili.hpp"
#include "fs/ActualFile.hpp"
#include "fs/VectorFile.hpp"

#include "err.hpp"
#include "applet_shim.hpp"

using namespace trn;

namespace twili {
namespace process {

AppletProcess::AppletProcess(Twili &twili, bridge::ResponseOpener attachment_opener, std::vector<uint8_t> nso) :
	MonitoredProcess(twili, attachment_opener) {

	// build virtual exefs
	virtual_exefs.SetRtld(std::make_shared<fs::ActualFile>(fopen("/squash/twili_applet_shim.nso", "rb")));
	virtual_exefs.SetNpdm(std::make_shared<fs::ActualFile>(fopen("/squash/default.npdm", "rb")));
	virtual_exefs.SetMain(std::make_shared<fs::VectorFile>(nso));
}

void AppletProcess::Launch() {
	// this pointer cast is not cool.
	twili.applet_tracker.QueueLaunch(std::dynamic_pointer_cast<AppletProcess>(shared_from_this()));
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

} // namespace process
} // namespace twili
