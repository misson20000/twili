#include "TwibPipe.hpp"

#include "err.hpp"

using trn::ResultError;

namespace twili {

TwibPipe::TwibPipe() {
}

TwibPipe::~TwibPipe() {
	Close();
}

void TwibPipe::Read(std::function<size_t(uint8_t *data, size_t actual_size)> cb) {
	switch(current_state_id) {
	case State::Idle:
		state_read_pending.cb = cb;
		current_state_id = State::ReadPending;
		break;
	case State::WritePending: {
		size_t read_size = cb(state_write_pending.data, state_write_pending.size);
		if(read_size < state_write_pending.size) {
			state_write_pending.data+= read_size;
			state_write_pending.size-= read_size;
		} else if(read_size == state_write_pending.size) {
			state_write_pending.cb(false);
			state_write_pending.cb = std::function<void(bool eof)>();
			current_state_id = State::Idle;
		} else {
			throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
		}
		break; }
	case State::ReadPending:
		throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
	case State::Closed:
		cb(nullptr, 0);
		break;
	default:
		throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
	}
}

void TwibPipe::Write(uint8_t *data, size_t size, std::function<void(bool eof)> cb) {
	if(size == 0) {
		return;
	}
	switch(current_state_id) {
	case State::Idle:
		state_write_pending.data = data;
		state_write_pending.size = size;
		state_write_pending.cb = cb;
		current_state_id = State::WritePending;
		break;
	case State::WritePending:
		throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
	case State::ReadPending: {
		current_state_id = State::Idle;
		size_t read_size = state_read_pending.cb(data, size);
		state_read_pending.cb = std::function<size_t(uint8_t *data, size_t actual_size)>();
		if(read_size < size) {
			state_write_pending.data = data + read_size;
			state_write_pending.size = size - read_size;
			state_write_pending.cb = cb;
			current_state_id = State::WritePending;
		} else if(read_size == size) {
			cb(false); // signal that we're finished
		} else {
			throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
		}
		break; }
	case State::Closed:
		cb(true);
		break;
	default:
		throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
	}
}

void TwibPipe::Close() {
	switch(current_state_id) {
	case State::WritePending:
		state_write_pending.cb(true);
		break;
	case State::ReadPending:
		state_read_pending.cb(nullptr, 0);
		break;
	default:
		break;
	}
	current_state_id = State::Closed;
}

} // namespace twili
