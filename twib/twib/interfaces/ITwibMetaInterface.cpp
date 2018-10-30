//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2018 misson20000 <xenotoad@xenotoad.net>
//
// This file is part of Twili.
//
// Twili is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Twili is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Twili.  If not, see <http://www.gnu.org/licenses/>.
//

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
