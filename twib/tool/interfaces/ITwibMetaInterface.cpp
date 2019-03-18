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
namespace tool {

ITwibMetaInterface::ITwibMetaInterface(RemoteObject obj) : obj(obj) {
}

std::vector<msgpack11::MsgPack> ITwibMetaInterface::ListDevices() {
	msgpack11::MsgPack ret;
	obj.SendSmartSyncRequest(
		CommandID::LIST_DEVICES,
		out(ret));
	return ret.array_items();
}

std::string ITwibMetaInterface::ConnectTcp(std::string hostname, std::string port) {
	std::string message;
	obj.SendSmartSyncRequest(
		CommandID::CONNECT_TCP,
		in(hostname),
		in(port),
		out(message));
	return message;
}

} // namespace tool
} // namespace twib
} // namespace twili
