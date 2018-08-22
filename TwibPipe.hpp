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
};

}
