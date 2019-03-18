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

#include "BridgeObject.hpp"

namespace twili {
namespace twib {
namespace daemon {

class Response {
 public:
	Response();
	Response(uint32_t client_id, uint32_t device_id, uint32_t object_id, uint32_t result_code, uint32_t tag, std::vector<uint8_t> payload);
	Response(uint32_t client_id, uint32_t device_id, uint32_t object_id, uint32_t result_code, uint32_t tag);
	
	uint32_t client_id;
	uint32_t device_id;
	uint32_t object_id;
	uint32_t result_code;
	uint32_t tag;
	std::vector<uint8_t> payload;
	std::vector<std::shared_ptr<BridgeObject>> objects;
};

class Client : public std::enable_shared_from_this<Client> {
 public:
	uint32_t client_id;
	bool deletion_flag = false;
	virtual void PostResponse(Response &r) = 0;
	std::vector<std::shared_ptr<BridgeObject>> owned_objects;
};

class WeakRequest {
 public:
	WeakRequest();
	WeakRequest(uint32_t client, uint32_t device_id, uint32_t object_id, uint32_t command_id, uint32_t tag, std::vector<uint8_t> payload);
	WeakRequest(uint32_t client, uint32_t device_id, uint32_t object_id, uint32_t command_id, uint32_t tag);
	Response RespondError(uint32_t code);
	Response RespondOk();
	
	uint32_t client_id;
	uint32_t device_id;
	uint32_t object_id;
	uint32_t command_id;
	uint32_t tag;
	std::vector<uint8_t> payload;
 private:
};

class Request {
 public:
	Request();
	Request(std::shared_ptr<Client> client, uint32_t device_id, uint32_t object_id, uint32_t command_id, uint32_t tag, std::vector<uint8_t> payload);
	Request(std::shared_ptr<Client> client, uint32_t device_id, uint32_t object_id, uint32_t command_id, uint32_t tag);
	Response RespondError(uint32_t code);
	Response RespondOk();
	WeakRequest Weak() const;
	
	std::shared_ptr<Client> client;
	uint32_t device_id;
	uint32_t object_id;
	uint32_t command_id;
	uint32_t tag;
	std::vector<uint8_t> payload;
 private:
};

} // namesapce daemon
} // namespace twib
} // namespace twili
