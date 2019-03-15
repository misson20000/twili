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

#include "NamedPipeMessageConnection.hpp"

namespace twili {
namespace twibc {

NamedPipeMessageConnection::NamedPipeMessageConnection(platform::windows::Pipe &&pipe, const EventThreadNotifier &notifier) :
	input_member(*this),
	output_member(*this),
	pipe(std::move(pipe)),
	notifier(notifier),
	out_buffer_lock(out_buffer_mutex, std::defer_lock) {
	if(this->pipe.handle == INVALID_HANDLE_VALUE) {
		LogMessage(Error, "invalid pipe");
		exit(1);
	}
}

NamedPipeMessageConnection::~NamedPipeMessageConnection() {

}

NamedPipeMessageConnection::InputMember::InputMember(NamedPipeMessageConnection &connection) : connection(connection) {
	if(event.handle == INVALID_HANDLE_VALUE) {
		LogMessage(Error, "invalid event");
		exit(1);
	}

	overlap.hEvent = event.handle;
}

NamedPipeMessageConnection::OutputMember::OutputMember(NamedPipeMessageConnection &connection) : connection(connection) {
	if(event.handle == INVALID_HANDLE_VALUE) {
		LogMessage(Error, "invalid event");
		exit(1);
	}

	overlap.hEvent = event.handle;
}

bool NamedPipeMessageConnection::InputMember::WantsSignal() {
	return connection.is_reading;
}

bool NamedPipeMessageConnection::OutputMember::WantsSignal() {
	return connection.is_writing;
}

void NamedPipeMessageConnection::InputMember::Signal() {
	LogMessage(Debug, "NPMC MessagePipe signalled in");
	std::lock_guard<std::mutex> guard(connection.state_mutex);

	if(!connection.is_reading) {
		// this should not happen
		LogMessage(Warning, "NPMC MessagePipe signalled in while not reading?");
		connection.error_flag = true;
		return;
	}

	DWORD bytes_transferred = 0;
	if(!GetOverlappedResult(connection.pipe.handle, &overlap, &bytes_transferred, false)) {
		LogMessage(Debug, "GetOverlappedResult failed: %d", GetLastError());
		connection.error_flag = true;
		return;
	}

	LogMessage(Debug, "got 0x%x bytes in", bytes_transferred);
	connection.in_buffer.MarkWritten(bytes_transferred);
	connection.is_reading = false;
}

void NamedPipeMessageConnection::OutputMember::Signal() {
	LogMessage(Debug, "NPMC MessagePipe signalled out");
	std::lock_guard<std::mutex> guard(connection.state_mutex);

	if(!connection.is_writing) {
		// this should not happen
		LogMessage(Warning, "NPMC MessagePipe signalled out while not writing?");
		connection.error_flag = true;
		return;
	}

	DWORD bytes_transferred = 0;
	if(!GetOverlappedResult(connection.pipe.handle, &overlap, &bytes_transferred, false)) {
		LogMessage(Debug, "GetOverlappedResult failed: %d", GetLastError());
		connection.error_flag = true;
		return;
	}

	LogMessage(Debug, "wrote 0x%x bytes", bytes_transferred);
	connection.out_buffer.MarkRead(bytes_transferred);
	connection.out_buffer_lock.unlock();
	connection.is_writing = false;
}

platform::windows::Event &NamedPipeMessageConnection::InputMember::GetEvent() {
	return event;
}

platform::windows::Event &NamedPipeMessageConnection::OutputMember::GetEvent() {
	return event;
}

bool NamedPipeMessageConnection::RequestInput() {
	LogMessage(Debug, "requesting input");
	std::lock_guard<std::mutex> guard(state_mutex);
	if(!is_reading) {
		LogMessage(Debug, "was in idle state");
		std::tuple<uint8_t*, size_t> target = in_buffer.Reserve(8192);
		DWORD bytes_read;
		if(ReadFile(pipe.handle, (void*)std::get<0>(target), std::get<1>(target), &bytes_read, &input_member.overlap)) {
			in_buffer.MarkWritten(bytes_read);
			LogMessage(Debug, "completed synchronously");
			return true;
		} else {
			if(GetLastError() != ERROR_IO_PENDING) {
				error_flag = true;
				LogMessage(Debug, "failed. GLE=%d", GetLastError());
				return false;
			}
			is_reading = true;
			LogMessage(Debug, "completing asynchronously");
			return false;
		}
	} else {
		LogMessage(Debug, "was not in idle state");
		return false;
	}
}

bool NamedPipeMessageConnection::RequestOutput() {
	LogMessage(Debug, "requesting output");
	std::lock_guard<std::mutex> guard(state_mutex);
	if(!is_writing) {
		out_buffer_lock.lock();
		LogMessage(Debug, "locked out_buffer_lock");
		if(out_buffer.ReadAvailable() > 0) {
			DWORD bytes_written;
			if(WriteFile(pipe.handle, (void*)out_buffer.Read(), out_buffer.ReadAvailable(), &bytes_written, &output_member.overlap)) {
				out_buffer.MarkRead(bytes_written);
				out_buffer_lock.unlock();
				LogMessage(Debug, "completed synchronously");
				return true;
			} else {
				if(GetLastError() != ERROR_IO_PENDING) {
					error_flag = true;
					out_buffer_lock.unlock();
					LogMessage(Debug, "failed");
					return false;
				}
				is_writing = true;
				LogMessage(Debug, "completing asynchronously");
				return false;
			}
		} else {
			out_buffer_lock.unlock();
		}
	} else {
		return false;
	}
}

} // namespace twibc
} // namespace twili
