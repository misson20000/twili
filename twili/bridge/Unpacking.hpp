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

#include "Buffer.hpp"
#include "err.hpp"

#include "Streaming.hpp"

namespace twili {
namespace bridge {
namespace detail {

template<typename T, class Enable = void>
struct UnpackingHelper;

// specialization for vectors of POD types
template<typename T>
struct UnpackingHelper<std::vector<T>, typename std::enable_if<std::is_pod<T>::value>::type> {
	bool Unpack(util::Buffer &buffer, std::vector<T> &out) {
		if(!has_size && !buffer.Read(size)) {
			return false;
		}
		has_size = true;
		std::vector<T> vec(size);
		if(!buffer.Read(vec)) {
			return false;
		}
		out = vec;
		return true;
	}
	
	static bool IsStream() {
		return false;
	}
private:
	size_t size = 0;
	bool has_size = false;
};

// specialization for strings
template<>
struct UnpackingHelper<std::string> {
	bool Unpack(util::Buffer &buffer, std::string &out) {
		std::vector<char> vec;
		if(helper.Unpack(buffer, vec)) {
			out = std::string(vec.begin(), vec.end());
			return true;
		} else {
			return false;
		}
	}

	static bool IsStream() {
		return false;
	}
 private:
	UnpackingHelper<std::vector<char>> helper;
};

// specialization for arbitrary vectors
template<typename T>
struct UnpackingHelper<std::vector<T>> {
	bool Unpack(util::Buffer &buffer, std::vector<T> &out) {
		if(!has_size) {
			if(!buffer.Read(size)) {
				return false;
			}
			out.clear();
			out.reserve(size);
			has_size = true;
		}
		
		while(out.size() < size) {
			T inst;
			if(!current_helper.Unpack(buffer)) {
				return false;
			}
			out.push_back(std::move(inst));
			current_helper = UnpackingHelper<T>();
		}
		
		return true;
	}
	
	static bool IsStream() {
		return false;
	}
 private:
	UnpackingHelper<T> current_helper;
	bool has_size;
	size_t size;
};

// specialization for streams
template<>
struct UnpackingHelper<InputStream> {
	bool Unpack(util::Buffer &buffer, InputStream &out) {
		return true;
	}
	static bool IsStream() {
		return true;
	}
};

// specialization for POD types
template<typename T>
struct UnpackingHelper<T, typename std::enable_if<std::is_pod<T>::value>::type> {
	bool Unpack(util::Buffer &buffer, T &out) {
		return buffer.Read(out);
	}
	static bool IsStream() {
		return false;
	}
};

template<typename... T>
struct ArgPack;

template<typename T>
struct UnpackingHolder;

template<typename T, typename... Args>
struct UnpackingHolder<ArgPack<T, Args...>> {
 public:
	bool Unpack(util::Buffer &buffer) {
		if(!has_value) {
			if(!helper.Unpack(buffer, value)) {
				return false;
			}
			has_value = true;
		}
		return next.Unpack(buffer);
	}
	std::tuple<T, Args...> GetValues() {
		return std::tuple_cat(std::make_tuple(value), next.GetValues());
	}
	InputStream *GetStream() {
		if constexpr(std::is_same<T, InputStream>::value) {
			return &value;
		}
		return next.GetStream();
	}
 private:
	bool has_value = false;
	T value;
	UnpackingHelper<T> helper;
	UnpackingHolder<ArgPack<Args...>> next;
};

template<>
struct UnpackingHolder<ArgPack<>> {
	bool Unpack(util::Buffer &buffer) {
		return true;
	}
	std::tuple<> GetValues() {
		return std::make_tuple();
	}
	InputStream *GetStream() {
		return nullptr;
	}
};

} // namespace detail
} // namespace bridge
} // namespace twili
