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

#include<type_traits>
#include<vector>
#include<tuple>

#include "../msgpack11/msgpack11.hpp"

#include "Buffer.hpp"
#include "err.hpp"

#include "ResponseWriter.hpp"

namespace twili {
namespace bridge {
namespace detail {

template<typename T, class Enable = void>
struct PackingHelper;

// specialization for vectors of POD types
template<typename T>
struct PackingHelper<std::vector<T>, typename std::enable_if<std::is_pod<T>::value>::type> {
	static size_t GetSize(std::vector<T> &&vec) {
		return vec.size() * sizeof(T) + sizeof(uint64_t);
	}

	static uint32_t GetObjectCount(std::vector<T> &&vec) {
		return 0;
	}
	
	static void Pack(std::vector<T> &&vec, ResponseWriter &w) {
		w.Write<uint64_t>(vec.size());
		w.Write(vec);
	}
};

// specialization for strings
template<>
struct PackingHelper<std::string> {
	static size_t GetSize(std::string &&string) {
		return sizeof(uint64_t) + string.size();
	}
	
	static uint32_t GetObjectCount(std::string &&val) {
		return 0;
	}

	static void Pack(std::string &&val, ResponseWriter &w) {
		PackingHelper<std::vector<char>>::Pack(std::vector<char>(val.begin(), val.end()), w);
	}
};

// specialization for arbitrary vectors
template<typename T>
struct PackingHelper<std::vector<T>> {
	static size_t GetSize(std::vector<T> &&vec) {
		size_t size = sizeof(uint64_t);
		for(auto i = vec.begin(); i != vec.end(); i++) {
			size+= PackingHelper<T>::GetSize(std::move(*i));
		}
		return size;
	}

	static uint32_t GetObjectCount(std::vector<T> &&vec) {
		uint32_t count = 0;
		for(auto i = vec.begin(); i != vec.end(); i++) {
			count+= PackingHelper<T>::GetObjectCount(std::move(*i));
		}
		return count;
	}
	
	static void Pack(std::vector<T> &&vec, ResponseWriter &w) {
		w.Write<uint64_t>(vec.size());
		for(auto i = vec.begin(); i != vec.end(); i++) {
			PackingHelper<T>::Pack(std::move(*i), w);
		}
	}
};

// specialization for bridge objects
template<>
struct PackingHelper<std::shared_ptr<Object>> {
	static size_t GetSize(std::shared_ptr<Object> &&val) {
		return sizeof(uint32_t);
	}

	static uint32_t GetObjectCount(std::shared_ptr<Object> &&val) {
		return 1;
	}

	static void Pack(std::shared_ptr<Object> &&val, ResponseWriter &w) {
		w.Write<uint32_t>(w.Object(val));
	}
};

// specialization for messagepack
template<>
struct PackingHelper<msgpack11::MsgPack> {
	static size_t GetSize(msgpack11::MsgPack &&val) {
		return sizeof(uint64_t) + val.dump().size();
	}

	static uint32_t GetObjectCount(msgpack11::MsgPack &&val) {
		return 0;
	}

	static void Pack(msgpack11::MsgPack &&val, ResponseWriter &w) {
		std::string ser = val.dump();
		w.Write<uint64_t>(ser.size());
		w.Write(ser);
	}
};

// specialization for POD types
template<typename T>
struct PackingHelper<T, typename std::enable_if<std::is_pod<T>::value>::type> {
	static size_t GetSize(T &&val) {
		return sizeof(T);
	}

	static uint32_t GetObjectCount(T &&val) {
		return 0;
	}

	static void Pack(T &&val, ResponseWriter &w) {
		w.Write<T>(val);
	}
};

} // namespace detail
} // namespace bridge
} // namespace twili
