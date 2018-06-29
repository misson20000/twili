#include "ITwibMetaInterface.hpp"

#include "Protocol.hpp"

namespace twili {
namespace twib {

ITwibMetaInterface::ITwibMetaInterface(RemoteObject obj) : obj(obj) {
}

std::vector<msgpack11::MsgPack> ITwibMetaInterface::ListDevices() {
	Response rs = obj.SendSyncRequest(protocol::ITwibMetaInterface::Command::LIST_DEVICES);
	std::string err;
	msgpack11::MsgPack obj = msgpack11::MsgPack::parse(std::string(rs.payload.begin(), rs.payload.end()), err);
	return obj.array_items();
}

} // namespace twib
} // namespace twili
