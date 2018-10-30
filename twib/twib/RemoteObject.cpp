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

#include "RemoteObject.hpp"

#include "Logger.hpp"
#include "ResultError.hpp"

namespace twili {
namespace twib {

RemoteObject::RemoteObject(client::Client &client, uint32_t device_id, uint32_t object_id) :
	client(client), device_id(device_id), object_id(object_id) {
	
}

RemoteObject::~RemoteObject() {
	// send close request if we're not object 0
	if(object_id != 0) {
		client.SendRequest(Request(device_id, object_id, 0xffffffff, 0, std::vector<uint8_t>()));
	}
}

std::future<Response> RemoteObject::SendRequest(uint32_t command_id, std::vector<uint8_t> payload) {
	return client.SendRequest(Request(device_id, object_id, command_id, 0, payload));
}

Response RemoteObject::SendSyncRequest(uint32_t command_id, std::vector<uint8_t> payload) {
	Response rs = SendRequest(command_id, payload).get();
	if(rs.result_code != 0) {
		throw ResultError(rs.result_code);
	}
	return rs;
}

} // namespace twib
} // namespace twili
