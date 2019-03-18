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

// voodoo
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

TwibPipe::TwibPipe(size_t buffer_limit) :
	buffer(buffer_limit) {
	TP_Debug("made TwibPipe(0x%lx)\n", buffer_limit);
}

TwibPipe::~TwibPipe() {
	CloseWriter();
}

const char *TwibPipe::StateName(state_variant &v) {
	switch(v.index()) {
	case 0:
		return "Idle";
	case 1:
		return "WritePending";
	case 2:
		return "ReadPending";
	default:
		return "Invalid";
	}
}

void TwibPipe::PrintDebugInfo(const char *indent) {
	printf("%shit_eof: %s\n", indent, hit_eof ? "true" : "false");
	printf("%sstate: %s\n", indent, StateName(state));
	std::visit(overloaded {
			[&](IdleState &idle) {
			},
			[&](WritePendingState &wps) {
				printf("%s  data: %p\n", indent, wps.data);
				printf("%s  size: 0x%lx\n", indent, wps.size);
			},
			[&](ReadPendingState &rps) {
			}
		}, state);
}

bool TwibPipe::IsWriterClosed() {
	return hit_eof;
}

TwibPipe::WritePendingState::WritePendingState(uint8_t *data, size_t size, std::function<void(bool eof)> cb) : data(data), size(size), cb(cb) {
}

TwibPipe::ReadPendingState::ReadPendingState(std::function<size_t(uint8_t*, size_t)> cb) : cb(cb) {
}

void TwibPipe::Read(std::function<size_t(uint8_t *data, size_t actual_size)> cb) {
	TP_Debug("TwibPipe(%s): Read\n", StateName(state));
	
	std::visit(overloaded {
			[&](IdleState &idle) {
				// Try to read from buffer.
				if(buffer.ReadAvailable() > 0) {
					TP_Debug("  buffer has 0x%lx bytes remaining\n", buffer.ReadAvailable());
					// There was buffered data, so send it to the read handler.
					size_t read_size = cb(buffer.Read(), buffer.ReadAvailable());
					// Mark how many bytes we read out of the buffer.
					buffer.MarkRead(read_size);
					TP_Debug("  read 0x%lx bytes from buffer\n", read_size);
				} else {
					if(hit_eof) {
						TP_Debug("  hit eof, signaling as such\n");
						cb(nullptr, 0);
					} else {
						// Didn't read, so enter read pending state.
						state.template emplace<ReadPendingState>(cb);
						TP_Debug("  entering read pending state\n");
					}
				}
			},
			[&](WritePendingState &wps) {
				TP_Debug("  trying to flush WPS to buffer\n");
				// Try to exit write-pending state by flushing to buffer.
				if(FlushWritePendingState(wps)) {
					// Re-enter if that worked, since we will have changed state.
					TP_Debug("  flushed, retrying...\n");
					return Read(cb);
				}

				TP_Debug("  trying to read from buffer before WPS (remaining 0x%lx)\n", buffer.ReadAvailable());
				
				// Try to read out of buffer.
				if(buffer.ReadAvailable() > 0) {
					// There was buffered data, so send it to the read handler.
					size_t read_size = cb(buffer.Read(), buffer.ReadAvailable());

					TP_Debug("  read 0x%lx\n", read_size);
					
					// Check if that made us exit WPS, and assert that if we did exit WPS,
					// it's because we hit EoF.
					if(!std::holds_alternative<WritePendingState>(state)) {
						if(!hit_eof) {
							throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
						} else {
							return;
						}
					}
					
					// Mark how many bytes we read out of the buffer.
					buffer.MarkRead(read_size);

					// Try to flush write-pending state to buffer.
					FlushWritePendingState(wps);

					// Don't need to re-enter, since we already used up this read callback.
				} else {
					TP_Debug ("  transferring directly from WPS\n");
					// Couldn't read from buffer.
					// Either limit = 0 (enforcing synchronous pipes), or wps' data
					// wouldn't fit in buffer. Read data directly from wps in that case.
					size_t read_size = cb(wps.data, wps.size);

					// Check if that made us exit WPS, and assert that if we did exit WPS,
					// it's because we closed.
					if(!std::holds_alternative<WritePendingState>(state)) {
						if(!hit_eof) {
							throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
						} else {
							return;
						}
					}

					// sanity
					if(read_size > wps.size) {
						throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
					}

					// Adjust WPS based on how much we read out.
					wps.data+= read_size;
					wps.size-= read_size;

					// Try to flush out WPS again, now that it's smaller.
					// If we read the whole thing, this will exit WPS.
					FlushWritePendingState(wps);
				}
			},
			[&](ReadPendingState &rps) {
				throw ResultError(TWILI_ERR_INVALID_PIPE_STATE);
			},
		}, state);
}

void TwibPipe::Write(uint8_t *data, size_t size, std::function<void(bool eof)> cb) {
	TP_Debug("TwibPipe(%s): Write(%p, 0x%lx)\n", StateName(state), data, size);

	// short-circuit zero-size writes so as not to confuse read handler
	if(size == 0 || hit_eof) {
		cb(hit_eof);
		return;
	}
	
	std::visit(overloaded {
			[&](IdleState &idle) {
				// Try to buffer and return immediately.
				if(buffer.Write(data, size)) {
					// Successfully stored in buffer, return immediately.
					cb(false);
				} else {
					// Buffer was full, need to wait.
					state.template emplace<WritePendingState>(data, size, cb);
				}
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
			}
		}, state);
}

void TwibPipe::CloseReader() {
	hit_eof = true;
	std::visit(overloaded {
			[&](IdleState &is) {
			},
			[&](WritePendingState &wps) {
				// signal EoF for writer
				wps.cb(true);
			},
			[&](ReadPendingState &rps) {
				// signal EoF for reader
				rps.cb(nullptr, 0);
			},
		}, state);
	state.template emplace<IdleState>();
}

void TwibPipe::CloseWriter() {
	hit_eof = true;
	std::visit(overloaded {
			[&](IdleState &is) {
			},
			[&](WritePendingState &wps) {
				// closing while you're writing is a strange thing to do...
				// signal EoF after pending data has been read out.
			},
			[&](ReadPendingState &rps) {
				// signal EoF for reader
				rps.cb(nullptr, 0);
				state.template emplace<IdleState>();
			},
		}, state);
}

bool TwibPipe::FlushWritePendingState(WritePendingState &wps) {
	// Try to transfer bytes from wps to buffer.
	std::tuple<uint8_t*, size_t> r = buffer.Reserve(wps.size);
	size_t allowed_transfer_size = std::min(std::get<1>(r), wps.size);
	std::copy_n(wps.data, allowed_transfer_size, std::get<0>(r));

	if(allowed_transfer_size < wps.size) {
		// didn't transfer everything, so adjust write pending state
		// and stay in it.
		wps.data+= std::get<1>(r);
		wps.size-= std::get<1>(r);
		return false;
	} else {
		ExitWritePendingState(wps);
		
		return true;
	}
}

void TwibPipe::ExitWritePendingState(WritePendingState &wps) {
	// move this out so we control lifetime, since we change state.
	std::function<void(bool eof)> write_cb = std::move(wps.cb);

	// Switch state before invoking write handler so that write handler
	// can change state if it wants.
	state.template emplace<IdleState>();
					
	write_cb(false);
}

} // namespace twili
