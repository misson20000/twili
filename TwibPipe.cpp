#include "TwibPipe.hpp"

#include "err.hpp"

using trn::ResultError;

#define TP_Debug(...) printf(__VA_ARGS__)

namespace twili {

TwibPipe::TwibPipe() {
}

TwibPipe::~TwibPipe() {
	Close();
}

const char *TwibPipe::StateName(State state) {
	switch(state) {
	case State::Idle:
		return "Idle";
	case State::WritePending:
		return "WritePending";
	case State::ReadPending:
		return "ReadPending";
	case State::Closed:
		return "Closed";
	default:
		return "Invalid";
	}
}

void TwibPipe::Read(std::function<size_t(uint8_t *data, size_t actual_size)> cb) {
	TP_Debug("TwibPipe(%s): Read\n", StateName(current_state_id));
	switch(current_state_id) {
	case State::Idle:
		// wait for someone to write
		state_read_pending.cb = cb;
		current_state_id = State::ReadPending;
		break;
	case State::WritePending: {
		// signal our async read handler that there was data waiting for us
		TP_Debug("  cb(%p, 0x%lx)\n", state_write_pending.data, state_write_pending.size);
		size_t read_size = cb(state_write_pending.data, state_write_pending.size);
		TP_Debug("    -> read 0x%lx bytes\n", read_size);
		if(read_size < state_write_pending.size) {
			// we didn't read everything, so adjust the write pending state
			// and stay in it
			state_write_pending.data+= read_size;
			state_write_pending.size-= read_size;
			TP_Debug("  adjusted write state(%p, 0x%lx)\n", state_write_pending.data, state_write_pending.size);
		} else if(read_size == state_write_pending.size) {
			// we read everything, so signal our write handler that it finished
			// writing and leave the write pending state.
			state_write_pending.cb(false);
			// clear this so it doesn't keep things alive
			state_write_pending.cb = std::function<void(bool eof)>();
			current_state_id = State::Idle;
			TP_Debug("  finished reading. entered Idle state\n");
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
	TP_Debug("TwibPipe(%s): Write(%p, 0x%lx)\n", StateName(current_state_id), data, size);
	
	switch(current_state_id) {
	case State::Idle:
		// wait for someone to read
		current_state_id = State::WritePending;
		state_write_pending.data = data;
		state_write_pending.size = size;
		state_write_pending.cb = cb;
		TP_Debug("  entered write state\n");
		break;
	case State::WritePending:
		throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
	case State::ReadPending: {
		// signal to our async read handler that we have data.
		size_t read_size = state_read_pending.cb(data, size);
		TP_Debug("  called read handler\n");
		TP_Debug("    -> 0x%lx bytes read\n", read_size);
		
		// clear this so it doesn't keep things alive.
		state_read_pending.cb = std::function<size_t(uint8_t *data, size_t actual_size)>();
		
		if(read_size < size) {
			// if we didn't read everything,
			// transition to write pending state.
			TP_Debug("  didn't read all data\n");
			current_state_id = State::WritePending;
			state_write_pending.data = data + read_size;
			state_write_pending.size = size - read_size;
			state_write_pending.cb = cb;
			TP_Debug("  entered write state(%p, 0x%lx)\n", state_write_pending.data, state_write_pending.size);
		} else if(read_size == size) {
			// if we read everything, we're idle
			current_state_id = State::Idle;
			cb(false); // signal our async write handler that it finished
			TP_Debug("  read everything\n");
		} else {
			// we read more data than we had?
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
