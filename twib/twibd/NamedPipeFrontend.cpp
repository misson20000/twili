#include "NamedPipeFrontend.hpp"

#include<algorithm>

#include "Twibd.hpp"

using namespace twili::platform::windows;

namespace twili {
namespace twibd {
namespace frontend {

NamedPipeFrontend::NamedPipeFrontend(Twibd *twibd, const char *name) : twibd(twibd) {
	pending_client = std::make_shared<Client>(*this);

	std::thread event_thread(&NamedPipeFrontend::event_thread_func, this);
	this->event_thread = std::move(event_thread);
}

NamedPipeFrontend::~NamedPipeFrontend() {
}

void NamedPipeFrontend::event_thread_func() {
	while(!event_thread_destroy) {
		std::vector<HANDLE> handles;
		std::vector<std::shared_ptr<Client>> active_clients;

		handles.push_back(pending_client->event.handle);
		active_clients.push_back(pending_client);
		for(auto &i : connections) {
			handles.push_back(i->event.handle);
			active_clients.push_back(i);
		}

		uint32_t r = WaitForMultipleObjects(handles.size(), handles.data(), false, INFINITE);
		if(r == WAIT_FAILED) {
			LogMessage(Error, "WaitForMultipleObjects failed: %d", GetLastError());
			exit(1);
		}
		if(r - WAIT_OBJECT_0 == 0) {
			LogMessage(Debug, "pending client signalled");
			pending_client->Signal();
		} else {
			active_clients[r - WAIT_OBJECT_0]->Signal();
		}

		if(pending_client->deletion_flag) {
			LogMessage(Error, "pending client error");
			pending_client = std::make_shared<Client>(*this);
		}

		for(auto i = connections.begin(); i != connections.end(); ) {
			(*i)->Process();
			
			if((*i)->deletion_flag) {
				twibd->RemoveClient((*i));
				i = connections.erase(i);
				continue;
			}

			i++;
		}
	}
}

void NamedPipeFrontend::ClientConnected(std::shared_ptr<twibd::Client> client) {
	if(client == pending_client) {
		connections.push_back(pending_client);
		pending_client = std::make_shared<Client>(*this);
	} else {
		LogMessage(Error, "unexpected client has connected");
	}
}

NamedPipeFrontend::Client::Client(NamedPipeFrontend &frontend) : frontend(frontend),
	event(nullptr, true, true, nullptr),
	pipe("\\\\.\\pipe\\twibd",
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
		PIPE_UNLIMITED_INSTANCES, 8192, 8192, 0, nullptr) {

	if(pipe.handle == INVALID_HANDLE_VALUE) {
		LogMessage(Error, "invalid pipe");
		exit(1);
	}

	if(event.handle == INVALID_HANDLE_VALUE) {
		LogMessage(Error, "invalid event");
		exit(1);
	}

	overlap.hEvent = event.handle;

	// attempt to connect
	if(ConnectNamedPipe(pipe.handle, &overlap)) {
		LogMessage(Error, "ConnectNamedPipe failed: %d", GetLastError());
		exit(1);
	}

	switch(GetLastError()) {
	case ERROR_IO_PENDING:
		break;
	case ERROR_PIPE_CONNECTED:
		Read();
		break;
	default:
		LogMessage(Error, "Invalid error code: %d", GetLastError());
		exit(1);
	}
}

NamedPipeFrontend::Client::~Client() {

}

void NamedPipeFrontend::Client::IncomingMessage(protocol::MessageHeader &mh, util::Buffer &payload, util::Buffer &object_ids) {
	LogMessage(Debug, "posting request");
	frontend.twibd->PostRequest(
		Request(
			shared_from_this(),
			mh.device_id,
			mh.object_id,
			mh.command_id,
			mh.tag,
			std::vector<uint8_t>(payload.Read(), payload.Read() + payload.ReadAvailable())));
	LogMessage(Debug, "posted request");
}

void NamedPipeFrontend::Client::PostResponse(Response &r) {
	std::lock_guard<std::mutex> lock(out_buffer_mutex);
	protocol::MessageHeader mh;
	mh.device_id = r.device_id;
	mh.object_id = r.object_id;
	mh.result_code = r.result_code;
	mh.tag = r.tag;
	mh.payload_size = r.payload.size();
	mh.object_count = r.objects.size();
	
	out_buffer.Write(mh);
	out_buffer.Write(r.payload);
	std::vector<uint32_t> object_ids(r.objects.size(), 0);
	std::transform(
		r.objects.begin(), r.objects.end(), object_ids.begin(),
		[](auto const &object) {
			return object->object_id;
		});
	out_buffer.Write(object_ids);

	Write();
}

void NamedPipeFrontend::Client::Signal() {
	DWORD bytes_transferred = 0;
	if(!GetOverlappedResult(pipe.handle, &overlap, &bytes_transferred, false)) {
		LogMessage(Debug, "GetOverlappedResult failed: %d", GetLastError());
		Error();
	}
	switch(state) {
	case State::Connecting:
		frontend.ClientConnected(shared_from_this());
		Process();
		break;
	case State::Reading:
		in_buffer.MarkWritten(bytes_transferred);
		Process();
		break;
	case State::Processing:
		break;
	case State::Writing:
		out_buffer.MarkRead(bytes_transferred);
		Process();
		break;
	case State::Error:
		break;
	}
}

void NamedPipeFrontend::Client::Read() {
	std::tuple<uint8_t*, size_t> target = in_buffer.Reserve(8192);
	DWORD actual_read;
	if(ReadFile(pipe.handle, (void*)std::get<0>(target), std::get<1>(target), &actual_read, &overlap)) {
		in_buffer.MarkWritten(actual_read);
		Process();
	} else {
		if(GetLastError() == ERROR_IO_PENDING) {
			state = State::Reading;
			return; // back to event loop
		} else {
			// failure
			Error();
		}
	}
}

void NamedPipeFrontend::Client::Write() {
	LogMessage(Debug, "pumping out 0x%lx bytes", out_buffer.ReadAvailable());
	std::lock_guard<std::mutex> lock(out_buffer_mutex);
	if(out_buffer.ReadAvailable() > 0) {
		DWORD actual_written;
		if(WriteFile(pipe.handle, (void*) out_buffer.Read(), out_buffer.ReadAvailable(), &actual_written, &overlap)) {
			out_buffer.MarkRead(actual_written);
			Process();
		} else {
			if(GetLastError() == ERROR_IO_PENDING) {
				state = State::Writing;
				return; // back to event loop
			} else {
				// failure
				Error();
			}
		}
	}
}

void NamedPipeFrontend::Client::Process() {
	ResetEvent(event.handle);
	if(out_buffer.ReadAvailable() > 0) {
		Write();
		return;
	}
	while(in_buffer.ReadAvailable() > 0) {
		if(!has_current_mh) {
			if(in_buffer.Read(current_mh)) {
				has_current_mh = true;
				current_payload.Clear();
				has_current_payload = false;
			} else {
				in_buffer.Reserve(sizeof(protocol::MessageHeader));
				Read();
				return;
			}
		}

		if(!has_current_payload) {
			if(in_buffer.Read(current_payload, current_mh.payload_size)) {
				has_current_payload = true;
				current_object_ids.Clear();
			} else {
				in_buffer.Reserve(current_mh.payload_size);
				Read();
				return;
			}
		}

		if(in_buffer.Read(current_object_ids, current_mh.object_count * sizeof(uint32_t))) {
			IncomingMessage(current_mh, current_payload, current_object_ids);
			has_current_mh = false;
			has_current_payload = false;
		} else {
			in_buffer.Reserve(current_mh.object_count * sizeof(uint32_t));
			Read();
			return;
		}
	}
}

void NamedPipeFrontend::Client::Error() {
	state = State::Error;
	deletion_flag = true;
	LogMessage(Debug, "client entering error state");
}

} // namespace frontend
} // namespace twibd
} // namespace twili