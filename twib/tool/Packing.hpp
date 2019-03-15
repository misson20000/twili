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

#include<optional>
#include<deque>

#include<msgpack11.hpp>

namespace twili {
namespace twib {
namespace tool {

template<typename T>
struct in {
	in(T &value) : value(std::move(value)) {
	}

	in(T &&value) : value(std::move(value)) {
	}
    
	T &&value;
};

template<typename T>
struct out {
	out(T &value) : value(std::move(value)) {
	}

	out(T &&value) : value(std::move(value)) {
	}
    
	T &&value;
};

template<typename T>
struct out_object {
	out_object(std::optional<T> &value) : value(value) {
	}

	std::optional<T> &value;
};

namespace detail {

template<typename T, class Enable = void>
struct PackingHelper;

template<typename T>
struct PackingHelper<std::vector<T>, typename std::enable_if<std::is_pod<T>::value>::type> {
	static void Pack(std::vector<T> &&value, util::Buffer &input_buffer) {
		input_buffer.Write<uint64_t>(value.size());
		input_buffer.Write(value);
	}
	static bool Unpack(std::vector<T> &&value, util::Buffer &output_buffer) {
		value.clear();
		uint64_t size;
		return output_buffer.Read<uint64_t>(size) && (value.resize(size), output_buffer.Read(value));
	}
};

template<typename T>
struct PackingHelper<std::vector<T>, typename std::enable_if<!std::is_pod<T>::value>::type> {
	static void Pack(std::vector<T> &&value, util::Buffer &input_buffer) {
		input_buffer.Write<uint64_t>(value.size());
		for(auto i = value.begin(); i != value.end(); i++) {
			PackingHelper<T>::Pack(std::move(*i), input_buffer);
		}
	}
	static bool Unpack(std::vector<T> &&value, util::Buffer &output_buffer) {
		value.clear();
		uint64_t size;
		if(!output_buffer.Read<uint64_t>(size)) {
			return false;
		}
		value.resize(size);
		for(size_t i = 0; i < size; i++) {
			if(!PackingHelper<T>::Unpack(std::move(value[i]), output_buffer)) {
				return false;
			}
		}
		return true;
	}
};

template<>
struct PackingHelper<std::string> {
	static void Pack(std::string &&value, util::Buffer &input_buffer) {
		input_buffer.Write<uint64_t>(value.size());
		input_buffer.Write(value);
	}
	static bool Unpack(std::string &&value, util::Buffer &output_buffer) {
		uint64_t size;
		return output_buffer.Read<uint64_t>(size) && output_buffer.Read(value, size);
	}
};

template<>
struct PackingHelper<msgpack11::MsgPack> {
	static void Pack(msgpack11::MsgPack &&value, util::Buffer &input_buffer) {
		std::string ser = value.dump();
		input_buffer.Write<uint64_t>(ser.size());
		input_buffer.Write(ser);
	}
	static bool Unpack(msgpack11::MsgPack &&value, util::Buffer &output_buffer) {
		uint64_t size;
		if(!output_buffer.Read<uint64_t>(size)) {
			return false;
		}
		std::string ser;
		if(!output_buffer.Read(ser, size)) {
			return false;
		}
		std::string err;
		value = msgpack11::MsgPack::parse(ser, err);
		return true;
	}
};

template<typename T>
struct PackingHelper<T, typename std::enable_if<std::is_pod<T>::value>::type> {
	static void Pack(T &&value, util::Buffer &input_buffer) {
		input_buffer.Write<T>(value);
	}
	static bool Unpack(T &&value, util::Buffer &output_buffer) {
		return output_buffer.Read<T>(value);
	}
};

template<typename T>
struct WrappingHelper;

template<typename T>
struct WrappingHelper<in<T>> {
	static void Pack(in<T> &&param, util::Buffer &input_buffer) {
		PackingHelper<T>::Pack(std::move(param.value), input_buffer);
	}
	static bool Unpack(in<T> &&param, util::Buffer &output_buffer, std::vector<std::shared_ptr<RemoteObject>> objects) {
		// no-op
		return true;
	}
};

template<typename T>
struct WrappingHelper<out<T>> {
	static void Pack(out<T> &&param, util::Buffer &input_buffer) {
		// no-op
	}
	static bool Unpack(out<T> &&param, util::Buffer &output_buffer, std::vector<std::shared_ptr<RemoteObject>> objects) {
		return PackingHelper<T>::Unpack(std::move(param.value), output_buffer);
	}
};

template<typename T>
struct WrappingHelper<out_object<T>> {
	static void Pack(out_object<T> &&param, util::Buffer &input_buffer) {
		// no-op
	}
	static bool Unpack(out_object<T> &&param, util::Buffer &output_buffer, std::vector<std::shared_ptr<RemoteObject>> objects) {
		uint32_t index;
		if(!output_buffer.Read(index)) {
			return false;
		}
		if(index >= objects.size()) {
			return false;
		}
		param.value.emplace(objects[index]);
		return true;
	}
};

} // namespace detail

} // namespace tool
} // namespace twib
} // namespace twili
