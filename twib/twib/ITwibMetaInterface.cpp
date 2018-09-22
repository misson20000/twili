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

std::string ITwibMetaInterface::ConnectTcp(std::string hostname, std::string port) {
	util::Buffer message;
	message.Write<size_t>(hostname.size());
	message.Write<size_t>(port.size());
	message.Write(hostname);
	message.Write(port);

	Response rs = obj.SendSyncRequest(protocol::ITwibMetaInterface::Command::CONNECT_TCP, message.GetData());
	return std::string(rs.payload.begin(), rs.payload.end());
}

} // namespace twib
} // namespace twili
