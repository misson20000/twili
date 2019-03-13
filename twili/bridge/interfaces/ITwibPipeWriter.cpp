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

#include "ITwibPipeWriter.hpp"

#include "err.hpp"

using trn::ResultCode;
using trn::ResultError;

namespace twili {
namespace bridge {

ITwibPipeWriter::ITwibPipeWriter(uint32_t device_id, std::shared_ptr<TwibPipe> pipe) : ObjectDispatcherProxy(*this, device_id), pipe(pipe), dispatcher(*this) {
}

ITwibPipeWriter::~ITwibPipeWriter() {
	pipe->CloseWriter();
}

void ITwibPipeWriter::Write(bridge::ResponseOpener opener, InputStream &stream) {
	if(pipe->IsWriterClosed()) {
		opener.RespondError(TWILI_ERR_EOF);
	}
	
	struct State : std::enable_shared_from_this<State> {
		State(bridge::ResponseOpener opener, std::shared_ptr<TwibPipe> pipe) :
			opener(opener), pipe(pipe) {
		}
		
		bridge::ResponseOpener opener;
		std::shared_ptr<TwibPipe> pipe;
		util::Buffer buffer;
		bool is_writing = false;
		bool is_done = false;
		
		void Write() {
			if(!buffer.ReadAvailable()) {
				if(is_done) {
					opener.RespondOk();
				}
				return;
			}
			
			is_writing = true;
			
			// move data to start of buffer so it doesn't get moved later
			buffer.Compact();
			
			size_t size = buffer.ReadAvailable();
			
			std::shared_ptr<State> extension = shared_from_this(); // extend our lifetime
			pipe->Write(
				buffer.Read(), size,
				[extension, size](bool eof) mutable {
					extension->buffer.MarkRead(size);
					extension->is_writing = false;
					if(eof) {
						if(!extension->is_done) {
							extension->is_done = true;
							extension->opener.RespondError(TWILI_ERR_EOF);
						}
					} else {
						if(extension->is_done && !extension->buffer.ReadAvailable()) {
							extension->opener.RespondOk();
						} else {
							extension->Write();
						}
					}
				});
		}
	};
	
	std::shared_ptr<State> state = std::make_shared<State>(opener, pipe);
	
	stream.receive =
		[state](util::Buffer &input) {
			size_t size = input.ReadAvailable();
			if(state->is_writing) {
				// if this flag is set, we have a reference to data in the buffer
				// and we don't want it to be moved
				size = std::min(size, state->buffer.WriteAvailableHint());
			}
			input.Read(state->buffer, std::min(input.ReadAvailable(), size));
			if(!state->is_writing) {
				state->Write();
			}
		};
	
	stream.finish =
		[state](util::Buffer &input) {
			state->is_done = true;
			if(!state->is_writing) {
				state->Write();
			}
		};
}

void ITwibPipeWriter::Close(bridge::ResponseOpener opener) {
	pipe->CloseWriter();
	opener.RespondOk();
}

} // namespace bridge
} // namespace twili
