#include "AppletProcess.hpp"

#include "../twili.hpp"

using namespace trn;

namespace twili {
namespace process {

AppletProcess::AppletProcess(Twili &twili, std::vector<uint8_t> nro) :
	MonitoredProcess(twili),
	reader(nro) {
	builder.AppendNRO(reader);
}

void AppletProcess::Launch() {
	twili.applet_tracker.QueueLaunch(shared_from_this());
}

void AppletProcess::Attach(trn::KProcess &&process) {
	proc = std::make_shared<trn::KProcess>(std::move(process));
}

size_t AppletProcess::GetTargetSize() {
	return builder.GetTotalSize();
}

void AppletProcess::SetupTarget(uint64_t buffer_address, uint64_t map_address) {
	ResultCode::AssertOk(builder.Load(proc, buffer_address, map_address));
}

void AppletProcess::FinalizeTarget() {
	ResultCode::AssertOk(builder.Unload(proc));
}

} // namespace process
} // namespace twili
