#pragma once

#include<libtransistor/cpp/types.hpp>

namespace twili {

// output-only
class TwibPipe {
 public:
	TwibPipe();
	~TwibPipe();
	void Read(std::function<void(uint8_t *data, size_t actual_size)> cb);
	void Write(uint8_t *data, size_t size, std::function<void()> cb);
 private:
	class IdleState {
	 public:
	};
	class WritePendingState {
	 public:
		uint8_t *data;
		size_t size;
		std::function<void()> cb;
	};
	class ReadPendingState {
	 public:
		std::function<void(uint8_t *data, size_t actual_size)> cb;
	};
	enum class State {
		Idle,
		WritePending,
		ReadPending,
	};
	State current_state_id = State::Idle;
	IdleState state_idle;
	WritePendingState state_write_pending;
	ReadPendingState state_read_pending;
};

}
