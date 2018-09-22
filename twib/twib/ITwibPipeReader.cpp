#include "ITwibPipeReader.hpp"

#include "Protocol.hpp"

namespace twili {
namespace twib {

ITwibPipeReader::ITwibPipeReader(std::shared_ptr<RemoteObject> obj) : obj(obj) {
	
}

std::vector<uint8_t> ITwibPipeReader::ReadSync() {
	return obj->SendSyncRequest(protocol::ITwibPipeReader::Command::READ).payload;
}

} // namespace twib
} // namespace twili
