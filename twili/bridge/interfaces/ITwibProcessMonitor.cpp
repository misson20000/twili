#include "ITwibProcessMonitor.hpp"

#include<libtransistor/cpp/nx.hpp>

#include "../../twili.hpp"
#include "../../process/MonitoredProcess.hpp"

#include "ITwibPipeReader.hpp"
#include "ITwibPipeWriter.hpp"

#include "err.hpp"

using namespace trn;

namespace twili {
namespace bridge {

ITwibProcessMonitor::ITwibProcessMonitor(uint32_t object_id, std::shared_ptr<process::MonitoredProcess> process) : Object(object_id), process(process) {
}

ITwibProcessMonitor::~ITwibProcessMonitor() {
	process->Kill(); // attempt to exit cleanly
}

void ITwibProcessMonitor::HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	switch((protocol::ITwibProcessMonitor::Command) command_id) {
	case protocol::ITwibProcessMonitor::Command::LAUNCH:
		Launch(payload, opener);
		break;
	case protocol::ITwibProcessMonitor::Command::TERMINATE:
		Terminate(payload, opener);
		break;
	case protocol::ITwibProcessMonitor::Command::APPEND_CODE:
		AppendCode(payload, opener);
		break;
	case protocol::ITwibProcessMonitor::Command::OPEN_STDIN:
		OpenStdin(payload, opener);
		break;
	case protocol::ITwibProcessMonitor::Command::OPEN_STDOUT:
		OpenStdout(payload, opener);
		break;
	case protocol::ITwibProcessMonitor::Command::OPEN_STDERR:
		OpenStderr(payload, opener);
		break;
	default:
		opener.BeginError(ResultCode(TWILI_ERR_PROTOCOL_UNRECOGNIZED_FUNCTION)).Finalize();
		break;
	}
}

void ITwibProcessMonitor::Launch(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() > 0) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}

	process->Launch(opener);
}

void ITwibProcessMonitor::Terminate(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() > 0) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}

	process->Terminate();
	opener.BeginOk().Finalize();
}

void ITwibProcessMonitor::AppendCode(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	process->AppendCode(payload);
	opener.BeginOk().Finalize();
}

void ITwibProcessMonitor::OpenStdin(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() > 0) { throw ResultError(TWILI_ERR_BAD_REQUEST); }
	auto w = opener.BeginOk(sizeof(uint32_t), 1);
	w.Write<uint32_t>(w.Object(opener.MakeObject<ITwibPipeWriter>(process->tp_stdin)));
	w.Finalize();
}

void ITwibProcessMonitor::OpenStdout(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() > 0) { throw ResultError(TWILI_ERR_BAD_REQUEST); }
	auto w = opener.BeginOk(sizeof(uint32_t), 1);
	w.Write<uint32_t>(w.Object(opener.MakeObject<ITwibPipeReader>(process->tp_stdout)));
	w.Finalize();
}

void ITwibProcessMonitor::OpenStderr(std::vector<uint8_t> payload, bridge::ResponseOpener opener) {
	if(payload.size() > 0) { throw ResultError(TWILI_ERR_BAD_REQUEST); }
	auto w = opener.BeginOk(sizeof(uint32_t), 1);
	w.Write<uint32_t>(w.Object(opener.MakeObject<ITwibPipeReader>(process->tp_stderr)));
	w.Finalize();
}

} // namespace bridge
} // namespace twili
