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

#include<vector>
#include<memory>

#include<stdint.h>

namespace twili {
namespace twib {
namespace client {
class Client;
}

class RemoteObject;

class Response {
 public:
	Response(uint32_t device_id, uint32_t object_id, uint32_t result_code, uint32_t tag, std::vector<uint8_t> payload, std::vector<std::shared_ptr<RemoteObject>> objects);
	Response() = default; // MSVC++ futures require this to be default constructible

	uint32_t device_id;
	uint32_t object_id;
	uint32_t result_code;
	uint32_t tag;
	std::vector<uint8_t> payload;
	std::vector<std::shared_ptr<RemoteObject>> objects;
};

class Request {
 public:
	Request();
	Request(uint32_t device_id, uint32_t object_id, uint32_t command_id, uint32_t tag, std::vector<uint8_t> payload);

	uint32_t device_id;
	uint32_t object_id;
	uint32_t command_id;
	uint32_t tag;
	std::vector<uint8_t> payload;
 private:
};

} // namespace twib
} // namespace twili
