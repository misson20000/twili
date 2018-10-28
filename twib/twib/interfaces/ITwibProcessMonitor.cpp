#include "ITwibProcessMonitor.hpp"

#include "Protocol.hpp"
#include "Logger.hpp"

namespace twili {
namespace twib {

ITwibProcessMonitor::ITwibProcessMonitor(std::shared_ptr<RemoteObject> obj) : obj(obj) {
}

uint64_t ITwibProcessMonitor::Launch() {
	Response rs = obj->SendSyncRequest(protocol::ITwibProcessMonitor::Command::LAUNCH);
	if(rs.payload.size() < sizeof(uint64_t)) {
		LogMessage(Fatal, "response size invalid");
		exit(1);
	}
	return *(uint64_t*) rs.payload.data();
}

void ITwibProcessMonitor::AppendCode(std::vector<uint8_t> code) {
	obj->SendSyncRequest(protocol::ITwibProcessMonitor::Command::APPEND_CODE, code);
}

ITwibPipeWriter ITwibProcessMonitor::OpenStdin() {
	return ITwibPipeWriter(obj->SendSyncRequest(protocol::ITwibProcessMonitor::Command::OPEN_STDIN).objects[0]);
}

ITwibPipeReader ITwibProcessMonitor::OpenStdout() {
	return ITwibPipeReader(obj->SendSyncRequest(protocol::ITwibProcessMonitor::Command::OPEN_STDOUT).objects[0]);
}

ITwibPipeReader ITwibProcessMonitor::OpenStderr() {
	return ITwibPipeReader(obj->SendSyncRequest(protocol::ITwibProcessMonitor::Command::OPEN_STDERR).objects[0]);
}

uint32_t ITwibProcessMonitor::WaitStateChange() {
	Response rs = obj->SendSyncRequest(protocol::ITwibProcessMonitor::Command::WAIT_STATE_CHANGE);
	if(rs.payload.size() < sizeof(uint32_t)) {
		LogMessage(Fatal, "response size invalid");
		exit(1);
	}
	return *(uint32_t*) rs.payload.data();
}

} // namespace twib
} // namespace twili
