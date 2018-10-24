#include "NamedPipeMessageConnection.hpp"

namespace twili {
namespace twibc {

NamedPipeMessageConnection::NamedPipeMessageConnection(platform::windows::Pipe &&pipe, const EventThreadNotifier &notifier) :
	pipe(*this, std::move(pipe)),
	notifier(notifier),
	out_buffer_lock(out_buffer_mutex, std::defer_lock) {
}

NamedPipeMessageConnection::NamedPipeMessageConnection(NamedPipeServer::Pipe &&pipe, const EventThreadNotifier &notifier) :
	pipe(*this, std::move(pipe)),
	notifier(notifier),
	out_buffer_lock(out_buffer_mutex, std::defer_lock) {
}

NamedPipeMessageConnection::~NamedPipeMessageConnection() {

}

NamedPipeMessageConnection::MessagePipe::MessagePipe(NamedPipeMessageConnection &connection, platform::windows::Pipe &&pipe) : Pipe(std::move(pipe)), connection(connection) {
	if(this->pipe.handle == INVALID_HANDLE_VALUE) {
		LogMessage(Error, "invalid pipe");
		exit(1);
	}

	if(event_in.handle == INVALID_HANDLE_VALUE) {
		LogMessage(Error, "invalid event");
		exit(1);
	}

	if(event_out.handle == INVALID_HANDLE_VALUE) {
		LogMessage(Error, "invalid event");
		exit(1);
	}
}

NamedPipeMessageConnection::MessagePipe::MessagePipe(NamedPipeMessageConnection &connection, Pipe &&other) : Pipe(std::move(other)), connection(connection) {
	if(this->pipe.handle == INVALID_HANDLE_VALUE) {
		LogMessage(Error, "invalid pipe");
		exit(1);
	}

	if(event_in.handle == INVALID_HANDLE_VALUE) {
		LogMessage(Error, "invalid event");
		exit(1);
	}

	if(event_out.handle == INVALID_HANDLE_VALUE) {
		LogMessage(Error, "invalid event");
		exit(1);
	}
}

bool NamedPipeMessageConnection::MessagePipe::WantsSignalIn() {
	return connection.is_reading;
}

bool NamedPipeMessageConnection::MessagePipe::WantsSignalOut() {
	return connection.is_writing;
}

void NamedPipeMessageConnection::MessagePipe::SignalIn() {
	LogMessage(Debug, "NPMC MessagePipe signalled in");
	std::lock_guard<std::mutex> guard(connection.state_mutex);

	if(!connection.is_reading) {
		// this should not happen
		LogMessage(Warning, "NPMC MessagePipe signalled in while not reading?");
		connection.error_flag = true;
		return;
	}

	DWORD bytes_transferred = 0;
	if(!GetOverlappedResult(pipe.handle, &overlap_in, &bytes_transferred, false)) {
		LogMessage(Debug, "GetOverlappedResult failed: %d", GetLastError());
		connection.error_flag = true;
		return;
	}

	LogMessage(Debug, "got 0x%x bytes in", bytes_transferred);
	connection.in_buffer.MarkWritten(bytes_transferred);
	connection.is_reading = false;
}

void NamedPipeMessageConnection::MessagePipe::SignalOut() {
	LogMessage(Debug, "NPMC MessagePipe signalled out");
	std::lock_guard<std::mutex> guard(connection.state_mutex);

	if(!connection.is_writing) {
		// this should not happen
		LogMessage(Warning, "NPMC MessagePipe signalled out while not writing?");
		connection.error_flag = true;
		return;
	}

	DWORD bytes_transferred = 0;
	if(!GetOverlappedResult(pipe.handle, &overlap_out, &bytes_transferred, false)) {
		LogMessage(Debug, "GetOverlappedResult failed: %d", GetLastError());
		connection.error_flag = true;
		return;
	}

	LogMessage(Debug, "wrote 0x%x bytes", bytes_transferred);
	connection.out_buffer.MarkRead(bytes_transferred);
	connection.out_buffer_lock.unlock();
	connection.is_writing = false;
}

bool NamedPipeMessageConnection::RequestInput() {
	LogMessage(Debug, "requesting input");
	std::lock_guard<std::mutex> guard(state_mutex);
	if(!is_reading) {
		LogMessage(Debug, "was in idle state");
		std::tuple<uint8_t*, size_t> target = in_buffer.Reserve(8192);
		DWORD bytes_read;
		if(ReadFile(pipe.pipe.handle, (void*)std::get<0>(target), std::get<1>(target), &bytes_read, &pipe.overlap_in)) {
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
			if(WriteFile(pipe.pipe.handle, (void*)out_buffer.Read(), out_buffer.ReadAvailable(), &bytes_written, &pipe.overlap_out)) {
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
