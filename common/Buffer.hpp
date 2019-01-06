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

#include<string>
#include<vector>
#include<type_traits>
#include<optional>
#include<tuple>

#include<stdint.h>

namespace twili {
namespace util {

class Buffer {
public:
	Buffer();
	Buffer(std::vector<uint8_t> data);
	~Buffer();

	// reserves at least `size` bytes, returns a tuple of
	// the write head pointer and a size of how many bytes
	// can be written. call MarkWritten to mark how many
	// bytes were actually read.
	std::tuple<uint8_t*, size_t> Reserve(size_t size);
	void MarkWritten(size_t size);
	
	void Write(const uint8_t *data, size_t size);
	
	template<typename T>
	void Write(std::vector<T> data) {
		static_assert(std::is_standard_layout<T>::value, "T must be standard layout");
		Write((uint8_t*) data.data(), sizeof(T) * data.size());
	}

	void Write(std::string &str);
	
	template<typename T>
	void Write(T t) {
		static_assert(std::is_standard_layout<T>::value, "T must be standard layout");
		Write((uint8_t*) &t, sizeof(T));
	}
	
	// if there is enough data in the buffer, it will overwrite
	// data and return true, otherwise data will be left alone and
	// it will return false.
	bool Read(uint8_t *data, size_t size);

	template<typename T>
	bool Read(T &out) {
		static_assert(std::is_standard_layout<T>::value, "T must be standard layout");
		return Read((uint8_t*) &out, sizeof(T));
	}

	template<typename T>
	bool Read(std::vector<T> &vec) {
		static_assert(std::is_standard_layout<T>::value, "T must be standard layout");
		T temp;
		return Read((uint8_t*) vec.data(), sizeof(T) * vec.size());
	}

	bool Read(std::string &str, size_t size);
	
	bool Read(Buffer &other, size_t size) {
		if(Read(std::get<0>(other.Reserve(size)), size)) {
			other.MarkWritten(size);
			return true;
		}
		return false;
	}

	uint8_t *Read();
	void MarkRead(size_t size);

	void Clear();
	
	size_t ReadAvailable();
	size_t WriteAvailableHint();

	std::vector<uint8_t> GetData();

	void Compact(); // guarantees that data pending read won't be moved around
private:
	std::vector<uint8_t> data;
	size_t read_head = 0;
	size_t write_head = 0;

	void EnsureSpace(size_t size);
};

} // namespace util
} // namespace twili
