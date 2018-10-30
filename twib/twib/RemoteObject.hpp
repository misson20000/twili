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

#pragma once

#include "Client.hpp"

namespace twili {
namespace twib {

class RemoteObject {
 public:
	RemoteObject(client::Client &client, uint32_t device_id, uint32_t object_id);
	~RemoteObject();

	std::future<Response> SendRequest(uint32_t command_id, std::vector<uint8_t> payload);
	Response SendSyncRequest(uint32_t command_id, std::vector<uint8_t> payload = std::vector<uint8_t>());

	template<typename T>
	Response SendSyncRequest(T command_id, std::vector<uint8_t> payload = std::vector<uint8_t>()) {
		return SendSyncRequest((uint32_t) command_id, payload);
	}
 private:
	client::Client &client;
	const uint32_t device_id;
	const uint32_t object_id;
};

} // namespace twib
} // namespace twili
