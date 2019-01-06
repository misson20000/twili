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

#include<libtransistor/cpp/types.hpp>

#include<memory>
#include<type_traits>
#include<vector>
#include<map>

#include "../common/Protocol.hpp"
#include "ResponseWriter.hpp"

namespace twili {
namespace bridge {

class Object;

namespace detail {

class ResponseState {
 public:
	inline ResponseState(uint32_t client_id, uint32_t tag) : client_id(client_id), tag(tag) {}
	virtual size_t GetMaxTransferSize() = 0;
	virtual void SendHeader(protocol::MessageHeader &hdr) = 0;
	virtual void SendData(uint8_t *data, size_t size) = 0;
	virtual void Finalize() = 0;
	virtual uint32_t ReserveObjectId() = 0;
	virtual void InsertObject(std::pair<uint32_t, std::shared_ptr<Object>> &&pair) = 0;

	const uint32_t client_id;
	const uint32_t tag;
	
	std::vector<std::shared_ptr<bridge::Object>> objects;
	size_t transferred_size = 0;
	size_t total_size = 0;
	uint32_t object_count = 0;
	bool has_begun = false;
};

} // namespace detail

class ResponseWriter {
 public:
	ResponseWriter(std::shared_ptr<detail::ResponseState> state);
	
	inline size_t GetMaxTransferSize() { return state->GetMaxTransferSize(); }
	void Write(uint8_t *data, size_t size);
	void Write(std::string str);
	
	template<typename T>
	void Write(std::vector<T> data) {
		static_assert(std::is_standard_layout<T>::value, "T must be standard layout");
		return Write((uint8_t*) data.data(), data.size() * sizeof(T));
	}
	
	template<typename T>
	void Write(T data) {
		static_assert(std::is_standard_layout<T>::value, "T must be standard layout");
		return Write((uint8_t*) &data, sizeof(data));
	}

	uint32_t Object(std::shared_ptr<bridge::Object> object);

	void Finalize();
 private:
	std::shared_ptr<detail::ResponseState> state;
};

} // namespace bridge
} // namespace twili
