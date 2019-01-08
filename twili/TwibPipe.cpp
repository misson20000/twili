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

#include "TwibPipe.hpp"

#include "err.hpp"

using trn::ResultError;

#define TP_Debug(...)
//#define TP_Debug(...) printf(__VA_ARGS__)

namespace twili {

TwibPipe::TwibPipe() {
}

TwibPipe::~TwibPipe() {
	Close();
}

bool TwibPipe::IsClosed() {
	return std::holds_alternative<ClosedState>(state);
}

const char *TwibPipe::StateName(state_variant &v) {
	switch(v.index()) {
	case 0:
		return "Idle";
	case 1:
		return "WritePending";
	case 2:
		return "ReadPending";
	case 3:
		return "Closed";
	default:
		return "Invalid";
	}
}

TwibPipe::WritePendingState::WritePendingState(uint8_t *data, size_t size, std::function<void(bool eof)> cb) : data(data), size(size), cb(cb) {
}

TwibPipe::ReadPendingState::ReadPendingState(std::function<size_t(uint8_t*, size_t)> cb) : cb(cb) {
}

// voodoo
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

void TwibPipe::Read(std::function<size_t(uint8_t *data, size_t actual_size)> cb) {
	TP_Debug("TwibPipe(%s): Read\n", StateName(state));

	std::visit(overloaded {
			[&](IdleState &idle) {
				state.template emplace<ReadPendingState>(cb);
			},
			[&](WritePendingState &wps) {
				// signal our async read handler that there was data waiting for us
				size_t read_size = cb(wps.data, wps.size);
				if(!std::holds_alternative<WritePendingState>(state)) {
					if(!IsClosed()) {
						throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
					} else {
						return;
					}
				}

				if(read_size < wps.size) {
					// we didn't read everything, so adjust the write pending state
					// and stay in it.
					wps.data+= read_size;
					wps.size-= read_size;
				} else if(read_size == wps.size) {
					// move this out so we control lifetime, since we change state.
					std::function<void(bool eof)> cb = std::move(wps.cb);

					// switch state before invoking write handler so that write handler
					// can change state if it wants.
					state.template emplace<IdleState>();
					
					// we read everything, so signal our write handler that it finished
					// writing.
					cb(false);
				} else {
					throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
				}
			},
			[&](ReadPendingState &rps) {
				throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
			},
			[&](ClosedState &c) {
				cb(nullptr, 0);
			},
		}, state);
}

void TwibPipe::Write(uint8_t *data, size_t size, std::function<void(bool eof)> cb) {
	TP_Debug("TwibPipe(%s): Write(%p, 0x%lx)\n", StateName(state), data, size);

	// short-circuit zero-size writes so as not to confuse read handler
	if(size == 0) {
		cb(IsClosed());
		return;
	}

	std::visit(overloaded {
			[&](IdleState &idle) {
				state.template emplace<WritePendingState>(data, size, cb);
			},
			[&](WritePendingState &wps) {
				throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
			},
			[&](ReadPendingState &rps) {
				// move this out so we control lifetime
				std::function<size_t(uint8_t*, size_t)> read_cb = std::move(rps.cb);

				// signal to async read handler that we have data
				size_t read_size = read_cb(data, size);

				if(read_size < size) {
					// if we didn't read everything,
					// transition to write pending state.
					state.template emplace<WritePendingState>(data + read_size, size - read_size, cb);
				} else if(read_size == size) {
					// if we read everything, enter idle state
					state.template emplace<IdleState>();
					// signal async write handler that we finished
					cb(false);
				} else {
					throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
				}
			},
			[&](ClosedState &cs) {
				throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
			},
		}, state);
}

void TwibPipe::Close() {
	std::visit(overloaded {
			[&](WritePendingState &wps) {
				wps.cb(true);
			},
			[&](ReadPendingState &rps) {
				rps.cb(nullptr, 0);
			},
			[&](auto &_) {
			},
		}, state);
	state.template emplace<ClosedState>();
}

} // namespace twili
