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

namespace twili {

class TwibPipe {
 public:
	TwibPipe();
	~TwibPipe();
	// callback returns how much data was read
	void Read(std::function<size_t(uint8_t *data, size_t actual_size)> cb);
	void Write(uint8_t *data, size_t size, std::function<void(bool eof)> cb);
	void Close();
 private:
	class IdleState {
	 public:
	};
	class WritePendingState {
	 public:
		uint8_t *data;
		size_t size;
		std::function<void(bool eof)> cb;
	};
	class ReadPendingState {
	 public:
		std::function<size_t(uint8_t *data, size_t actual_size)> cb;
	};
	enum class State {
		Idle,
		WritePending,
		ReadPending,
		Closed,
	};
	State current_state_id = State::Idle;
	IdleState state_idle;
	WritePendingState state_write_pending;
	ReadPendingState state_read_pending;

	static const char *StateName(State state);
};

}
