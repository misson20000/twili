#include "ITwibPipeWriter.hpp"

#include "err.hpp"

using trn::ResultCode;
using trn::ResultError;

namespace twili {
namespace bridge {

ITwibPipeWriter::ITwibPipeWriter(uint32_t device_id, std::weak_ptr<TwibPipe> pipe) : Object(device_id), pipe(pipe) {
}

void ITwibPipeWriter::HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
	switch((protocol::ITwibPipeWriter::Command) command_id) {
	case protocol::ITwibPipeWriter::Command::WRITE:
		Write(payload, opener);
		break;
	case protocol::ITwibPipeWriter::Command::CLOSE:
		Close(payload, opener);
		break;
	default:
		opener.BeginError(ResultCode(TWILI_ERR_PROTOCOL_UNRECOGNIZED_FUNCTION)).Finalize();
		break;
	}
}

void ITwibPipeWriter::Write(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
	if(std::shared_ptr<TwibPipe> observe = pipe.lock()) {
		// we need this so that payload will stay alive until our callback gets called
		std::shared_ptr<std::vector<uint8_t>> payload_copy = std::make_shared<std::vector<uint8_t>>(payload);
		observe->Write(payload_copy->data(), payload_copy->size(),
			[opener, payload_copy](bool eof) mutable {
				if(eof) {
					opener.BeginError(TWILI_ERR_EOF).Finalize();
				} else {
					opener.BeginOk().Finalize();
				}
				payload_copy.reset();
			});
	} else {
		opener.BeginError(TWILI_ERR_EOF).Finalize();
	}
}

void ITwibPipeWriter::Close(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
	if(payload.size() != 0) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}
	if(std::shared_ptr<TwibPipe> observe = pipe.lock()) {
		observe->Close();
		pipe.reset();
	}
}

} // namespace bridge
} // namespace twili
