#include "ITwibPipeWriter.hpp"

#include "Protocol.hpp"

namespace twili {
namespace twib {

ITwibPipeWriter::ITwibPipeWriter(std::shared_ptr<RemoteObject> obj) : obj(obj) {
	
}

void ITwibPipeWriter::WriteSync(std::vector<uint8_t> data) {
	obj->SendSyncRequest(protocol::ITwibPipeWriter::Command::WRITE, data);
}

void ITwibPipeWriter::Close() {
	obj->SendSyncRequest(protocol::ITwibPipeWriter::Command::CLOSE);
}

} // namespace twib
} // namespace twili
