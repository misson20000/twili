#include "ITwibMetaInterface.hpp"

#include "Protocol.hpp"

namespace twili {
namespace twib {

ITwibMetaInterface::ITwibMetaInterface(RemoteObject obj) : obj(obj) {
}

std::vector<msgpack11::MsgPack> ITwibMetaInterface::ListDevices() {
	std::future<twili::twib::Response> rs_future = obj.SendRequest((uint32_t) protocol::ITwibMetaInterface::Command::LIST_DEVICES, std::vector<uint8_t>());
	rs_future.wait();
	twili::twib::Response rs = rs_future.get();
	std::string err;
	msgpack11::MsgPack obj = msgpack11::MsgPack::parse(std::string(rs.payload.begin(), rs.payload.end()), err);
	return obj.array_items();
}

} // namespace twib
} // namespace twili
