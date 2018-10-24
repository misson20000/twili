#include "NamedPipeServer.hpp"

#include<algorithm>

#include "Logger.hpp"

namespace twili {
namespace twibc {

NamedPipeServer::NamedPipeServer(Logic &logic) : notifier(*this), logic(logic) {

}

NamedPipeServer::~NamedPipeServer() {
	Destroy();
	event_thread.join();
}

NamedPipeServer::Pipe::Pipe() : pipe(), event_in(nullptr, true, true, nullptr), event_out(nullptr, true, true, nullptr) {
	overlap_in.hEvent = this->event_in.handle;
	overlap_out.hEvent = this->event_out.handle;
}

NamedPipeServer::Pipe::Pipe(platform::windows::Pipe &&pipe) : pipe(std::move(pipe)), event_in(nullptr, true, true, nullptr), event_out(nullptr, true, true, nullptr) {
	overlap_in.hEvent = this->event_in.handle;
	overlap_out.hEvent = this->event_out.handle;
}

NamedPipeServer::Pipe::Pipe(Pipe &&other) : pipe(std::move(other.pipe)), event_in(std::move(other.event_in)), event_out(std::move(other.event_out)) {
	overlap_in.hEvent = this->event_in.handle;
	overlap_out.hEvent = this->event_out.handle;
}

NamedPipeServer::Pipe::~Pipe() {
}

NamedPipeServer::Pipe &NamedPipeServer::Pipe::operator=(Pipe &&other) {
	this->pipe = std::move(other.pipe);
	this->event_in = std::move(other.event_in);
	this->event_out = std::move(other.event_out);
	overlap_in.hEvent = this->event_in.handle;
	overlap_out.hEvent = this->event_out.handle;
	return *this;
}

bool NamedPipeServer::Pipe::WantsSignalIn() {
	return false;
}

bool NamedPipeServer::Pipe::WantsSignalOut() {
	return false;
}

void NamedPipeServer::Pipe::SignalIn() {
}

void NamedPipeServer::Pipe::SignalOut() {
}

void NamedPipeServer::Pipe::Close() {
	this->pipe.Close();
}

void NamedPipeServer::Begin() {
	std::thread event_thread(&NamedPipeServer::event_thread_func, this);
	this->event_thread = std::move(event_thread);
}

void NamedPipeServer::Destroy() {
	event_thread_destroy = true;
	notifier.Notify();
}

void NamedPipeServer::Clear() {
	pipes.clear();
}

void NamedPipeServer::AddPipe(Pipe &pipe) {
	pipes.push_back(pipe);
}

NamedPipeServer::EventThreadNotifier::EventThreadNotifier(NamedPipeServer &server) : server(server) {
}

void NamedPipeServer::EventThreadNotifier::Notify() const {
	if(!SetEvent(server.notification_event.handle)) {
		LogMessage(Fatal, "failed to set notification event");
		exit(1);
	}
}

void NamedPipeServer::event_thread_func() {
	while(!event_thread_destroy) {
		LogMessage(Debug, "socket event thread loop");

		logic.Prepare(*this);
		
		std::sort(pipes.begin(), pipes.end(), [](Pipe &a, Pipe &b) { return a.last_service > b.last_service; });
		std::vector<HANDLE> event_handles;
		std::vector<std::vector<std::reference_wrapper<Pipe>>::iterator> assoc_pipes;
		std::vector<bool> assoc_dirs;
		event_handles.push_back(notification_event.handle);
		assoc_pipes.push_back(pipes.end());
		assoc_dirs.push_back(false);

		for(std::vector<std::reference_wrapper<Pipe>>::iterator i = pipes.begin(); i != pipes.end(); i++) {
			if(i->get().WantsSignalIn()) {
				event_handles.push_back(i->get().event_in.handle);
				assoc_pipes.push_back(i);
				assoc_dirs.push_back(false);
			}
			if(i->get().WantsSignalOut()) {
				event_handles.push_back(i->get().event_out.handle);
				assoc_pipes.push_back(i);
				assoc_dirs.push_back(true);
			}
		}
		LogMessage(Debug, "waiting on %d handles", event_handles.size());
		DWORD r = WaitForMultipleObjects(event_handles.size(), event_handles.data(), false, INFINITE);
		LogMessage(Debug, "  -> %d", r);
		if(r == WAIT_FAILED || r < WAIT_OBJECT_0 || r - WAIT_OBJECT_0 >= assoc_pipes.size()) {
			LogMessage(Fatal, "WaitForMultipleObjects failed");
			exit(1);
		}
		if(r == WAIT_OBJECT_0) { // notification event
			continue;
		}
		assoc_pipes[r - WAIT_OBJECT_0]->get().last_service = 0;
		for(auto i = assoc_pipes[r - WAIT_OBJECT_0] + 1; i != pipes.end(); i++) {
			i->get().last_service++; // increment age on sockets that didn't get a change to signal
		}
		if(assoc_dirs[r - WAIT_OBJECT_0]) {
			assoc_pipes[r - WAIT_OBJECT_0 ]->get().SignalOut();
		} else {
			assoc_pipes[r - WAIT_OBJECT_0]->get().SignalIn();
		}
	}
}

} // namespace twibc
} // namespace twili
