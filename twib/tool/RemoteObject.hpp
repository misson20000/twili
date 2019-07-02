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

#include<functional>

#include "common/ResultError.hpp"

#include "err.hpp"
#include "Client.hpp"
#include "Packing.hpp"

namespace twili {
namespace twib {
namespace tool {

class RemoteObject {
 public:
	RemoteObject(client::Client &client, uint32_t device_id, uint32_t object_id);
	~RemoteObject();

	void SendRequest(uint32_t command_id, std::vector<uint8_t> payload, std::function<void(Response)> &&func);
	Response SendSyncRequestWithoutAssert(uint32_t command_id, std::vector<uint8_t> payload = std::vector<uint8_t>());
	Response SendSyncRequest(uint32_t command_id, std::vector<uint8_t> payload = std::vector<uint8_t>());

	template<typename T, typename... Args>
	uint32_t SendSmartSyncRequestWithoutAssert(T command_id, Args&&... args) {
		util::Buffer input_buffer;
		(detail::WrappingHelper<Args>::Pack(std::move(args), input_buffer), ...);
		Response r = SendSyncRequestWithoutAssert((uint32_t) command_id, input_buffer.GetData());
		if(r.result_code) {
			return r.result_code;
		}
		util::Buffer output_buffer(r.payload);
		if(!(detail::WrappingHelper<Args>::Unpack(std::move(args), output_buffer, r.objects) && ... && true)) {
			throw ResultError(TWILI_ERR_PROTOCOL_BAD_RESPONSE);
		}
		return 0;
	}

	template<typename T, typename... Args>
	void SendSmartSyncRequest(T command_id, Args&&... args) {
		uint32_t r;
		if((r = SendSmartSyncRequestWithoutAssert(command_id, std::move(args)...))) {
			throw ResultError(r);
		}
	}

	template<typename T, typename... Args>
	void SendSmartRequest(T command_id, std::function<void(uint32_t)> &&func, Args&&... args) {
		util::Buffer input_buffer;
		(detail::WrappingHelper<Args>::Pack(std::move(args), input_buffer), ...);
		SendRequest(
			(uint32_t) command_id,
			input_buffer.GetData(),
			[&, func{std::move(func)}](Response r) {
				if(r.result_code) {
					func(r.result_code);
				}
				util::Buffer output_buffer(r.payload);
				if(!(detail::WrappingHelper<Args>::Unpack(std::move(args), output_buffer, r.objects) && ... && true)) {
					func(TWILI_ERR_PROTOCOL_BAD_RESPONSE);
				} else {
					func(0);
				}
			});
	}
	
 private:
	client::Client &client;
	const uint32_t device_id;
	const uint32_t object_id;
};

} // namespace tool
} // namespace twib
} // namespace twili
