#include "UnmonitoredProcess.hpp"

#include<libtransistor/cpp/nx.hpp>

#include "../twili.hpp"

using namespace trn;

namespace twili {
namespace process {

UnmonitoredProcess::UnmonitoredProcess(Twili &twili, uint64_t pid) : Process(twili), pid(pid) {
}

uint64_t UnmonitoredProcess::GetPid() {
	return pid;
}

void UnmonitoredProcess::Terminate() {
	// try to terminate a process via pm:shell
	if(twili.services.pm_shell.TerminateProcessByPid(pid)) {
		return;
	}

	// try to terminate a process via svcTerminateDebugProcess as a last resort
	trn::KDebug debug = ResultCode::AssertOk(trn::svc::DebugActiveProcess(pid));
	ResultCode::AssertOk(svcTerminateDebugProcess(debug.handle));
}

} // namespace process
} // namespace twili
