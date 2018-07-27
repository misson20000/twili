#include "TwibPipe.hpp"

#include "err.hpp"

using trn::ResultError;

namespace twili {

TwibPipe::TwibPipe() {
}

TwibPipe::~TwibPipe() {
	printf("destroying twib pipe\n");
	switch(current_state_id) {
	case State::WritePending:
		state_write_pending.cb();
		break;
	case State::ReadPending:
		state_read_pending.cb(nullptr, 0);
		break;
	default:
		break;
	}
}

void TwibPipe::Read(std::function<void(uint8_t *data, size_t actual_size)> cb) {
	printf("TwibPipe: read state %d\n", (int) current_state_id);
	switch(current_state_id) {
	case State::Idle:
		printf("TwibPipe: reading idle\n");
		state_read_pending.cb = cb;
		current_state_id = State::ReadPending;
		break;
	case State::WritePending:
		printf("TwibPipe: reading with pending write\n");
		cb(state_write_pending.data, state_write_pending.size);
		state_write_pending.cb();
		state_write_pending.cb = std::function<void()>();
		current_state_id = State::Idle;
		break;
	case State::ReadPending:
	default:
		throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
	}
}

void TwibPipe::Write(uint8_t *data, size_t size, std::function<void()> cb) {
	printf("TwibPipe: write state %d size 0x%lx\n", (int) current_state_id, size);
	if(size == 0) {
		printf("size is zero\n");
		return;
	}
	switch(current_state_id) {
	case State::Idle:
		printf("TwibPipe: writing idle\n");
		state_write_pending.data = data;
		state_write_pending.size = size;
		state_write_pending.cb = cb;
		current_state_id = State::WritePending;
		break;
	case State::WritePending:
	default:
		throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
	case State::ReadPending:
		printf("TwibPipe: writing with pending read\n");
		current_state_id = State::Idle;
		state_read_pending.cb(data, size);
		state_read_pending.cb = std::function<void(uint8_t *data, size_t actual_size)>();
		cb();
		break;
	}
}

} // namespace twili
