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

#include "GdbStub.hpp"

#include<functional>

#include "Logger.hpp"
#include "ResultError.hpp"

namespace twili {
namespace twib {
namespace gdb {

GdbStub::GdbStub(ITwibDeviceInterface &itdi) :
	itdi(itdi),
	connection(STDIN_FILENO, STDOUT_FILENO),
	logic(*this),
	server(logic) {
	AddGettableQuery(Query(*this, "Supported", &GdbStub::QueryGetSupported, false));
	AddGettableQuery(Query(*this, "C", &GdbStub::QueryGetCurrentThread, false));
	AddSettableQuery(Query(*this, "StartNoAckMode", &GdbStub::QuerySetStartNoAckMode));
	AddMultiletterHandler("Attach", &GdbStub::HandleVAttach);
}

GdbStub::~GdbStub() {
	server.Destroy();
}

void GdbStub::Run() {
	std::unique_lock<std::mutex> lock(connection.mutex);
	server.Begin();
	while(!connection.error_flag) {
		connection.error_condvar.wait(lock);
	}
}

GdbStub::Query::Query(GdbStub &stub, std::string field, void (GdbStub::*visitor)(util::Buffer&), bool should_advertise) :
	stub(stub),
	field(field),
	visitor(visitor),
	should_advertise(should_advertise) {
}

void GdbStub::AddFeature(std::string feature) {
	features.push_back(feature);
}

void GdbStub::AddGettableQuery(Query query) {
	gettable_queries.emplace(query.field, query);

	if(query.should_advertise) {
		features.push_back(std::string("q") + query.field + "+");
	}
}

void GdbStub::AddSettableQuery(Query query) {
	settable_queries.emplace(query.field, query);

	if(query.should_advertise) {
		features.push_back(std::string("Q") + query.field + "+");
	}
}

void GdbStub::AddMultiletterHandler(std::string name, void (GdbStub::*handler)(util::Buffer&)) {
	multiletter_handlers.emplace(name, handler);
}

void GdbStub::HandleGeneralGetQuery(util::Buffer &packet) {
	std::string field;
	char ch;
	while(packet.Read(ch) && ch != ':') {
		field.push_back(ch);
	}
	LogMessage(Debug, "got get query for '%s'", field.c_str());

	auto i = gettable_queries.find(field);
	if(i != gettable_queries.end()) {
		std::invoke(i->second.visitor, this, packet);
	} else {
		LogMessage(Info, "unsupported query: '%s'", field.c_str());
		connection.RespondEmpty();
	}
}

void GdbStub::HandleGeneralSetQuery(util::Buffer &packet) {
	std::string field;
	char ch;
	while(packet.Read(ch) && ch != ':') {
		field.push_back(ch);
	}
	LogMessage(Debug, "got set query for '%s'", field.c_str());

	auto i = settable_queries.find(field);
	if(i != settable_queries.end()) {
		std::invoke(i->second.visitor, this, packet);
	} else {
		LogMessage(Info, "unsupported query: '%s'", field.c_str());
		connection.RespondEmpty();
	}
}

void GdbStub::HandleMultiletterPacket(util::Buffer &packet) {
	std::string title;
	char ch;
	while(packet.Read(ch) && ch != ';' &&  ch != '?') {
		title.push_back(ch);
	}
	LogMessage(Debug, "got v'%s'", title.c_str());

	auto i = multiletter_handlers.find(title);
	if(i != multiletter_handlers.end()) {
		std::invoke(i->second, this, packet);
	} else {
		LogMessage(Info, "unsupported v: '%s'", title.c_str());
		connection.RespondEmpty();
	}
}

void GdbStub::HandleGetStopReason() {
	util::Buffer buf;
	buf.Write(stop_reason);
	connection.Respond(buf);
}

void GdbStub::HandleSetCurrentThread(util::Buffer &packet) {
	if(packet.ReadAvailable() < 2) {
		LogMessage(Warning, "invalid thread id");
		connection.RespondError(1);
		return;
	}

	char op;
	if(!packet.Read(op)) {
		LogMessage(Warning, "invalid H packet");
		connection.RespondError(1);
		return;
	}

	uint64_t pid = current_thread ? current_thread->process.pid : 0;
	
	char ch;
	if(packet.ReadAvailable() && packet.Read()[0] == 'p') { // peek
		packet.MarkRead(1); // consume
		pid = 0;
		while(packet.ReadAvailable() >= 2 && packet.Read()[0] != '.') {
			pid<<= 8;
			pid|= GdbConnection::DecodeHexByte((char*) packet.Read());
			packet.MarkRead(2); // consume
		}
		if(packet.ReadAvailable()) {
			packet.MarkRead(1); // consume separator
		}
	}

	uint64_t thread_id = 0;
	
	while(packet.ReadAvailable() >= 2) {
		thread_id<<= 8;
		thread_id|= GdbConnection::DecodeHexByte((char*) packet.Read());
		packet.MarkRead(2); // consume
	}

	auto i = attached_processes.find(pid);
	if(i == attached_processes.end()) {
		LogMessage(Debug, "no such process with pid 0x%x", pid);
		connection.RespondError(1);
		return;
	}

	if(thread_id == 0) {
		// pick the first thread
		current_thread = &i->second.threads.begin()->second;
	} else {
		auto j = i->second.threads.find(thread_id);
		if(j == i->second.threads.end()) {
			LogMessage(Debug, "no such thread with tid 0x%x", thread_id);
			connection.RespondError(1);
			return;
		}
		
		current_thread = &j->second;
	}
	
	LogMessage(Debug, "selected thread for '%c': pid 0x%lx tid 0x%lx", op, pid, current_thread->thread_id);
	connection.RespondOk();
}

void GdbStub::HandleVAttach(util::Buffer &packet) {
	uint64_t pid = 0;
	char ch;
	while(packet.Read(ch)) {
		pid<<= 4;
		pid|= GdbConnection::DecodeHexNybble(ch);
	}
	LogMessage(Debug, "decoded PID: 0x%lx", pid);

	if(attached_processes.find(pid) != attached_processes.end()) {
		connection.RespondError(1);
		return;
	}

	auto r = attached_processes.emplace(pid, Process(pid, itdi.OpenActiveDebugger(pid)));
	ProcessEvents(r.first->second);
	
	stop_reason = "S05";
	
	// ok
	HandleGetStopReason();
}

void GdbStub::QueryGetSupported(util::Buffer &packet) {
	util::Buffer response;

	while(packet.ReadAvailable()) {
		char ch;
		std::string feature;
		while(packet.Read(ch) && ch != ';') {
			feature.push_back(ch);
		}

		LogMessage(Debug, "gdb advertises feature: '%s'", feature.c_str());
	}

	bool is_first = true;
	for(std::string &feature : features) {
		if(!is_first) {
			response.Write(';');
		} else {
			is_first = false;
		}
		
		response.Write(feature);
	}
	
	connection.Respond(response);
}

void GdbStub::QueryGetCurrentThread(util::Buffer &packet) {
	util::Buffer response;
	response.Write('p');
	GdbConnection::Encode(current_thread ? current_thread->process.pid : 0, 0, response);
	response.Write('.');
	GdbConnection::Encode(current_thread ? current_thread->thread_id : 0, 0, response);
	connection.Respond(response);
}

void GdbStub::QuerySetStartNoAckMode(util::Buffer &packet) {
	connection.StartNoAckMode();
	connection.RespondOk();
}

void GdbStub::ProcessEvents(Process &process) {
	try {
		while(1) {
			nx::DebugEvent event = process.debugger.GetDebugEvent();
			LogMessage(Debug, "got event: %d", event.event_type);

			switch(event.event_type) {
			case nx::DebugEvent::EventType::AttachProcess:
				break;
			case nx::DebugEvent::EventType::AttachThread:
				uint64_t thread_id = event.attach_thread.thread_id;
				LogMessage(Debug, "  attaching new thread: 0x%x", thread_id);
				auto r = process.threads.emplace(thread_id, Thread(process, thread_id));
				current_thread = &r.first->second;
				break;
			}
		}
	} catch(ResultError &e) {
		if(e.code == 0x8c01) {
			LogMessage(Debug, "got all events");
			return; // no events left
		} else {
			LogMessage(Debug, "error while processing events: 0x%x", e.code);
			throw e; // propogate
		}
	}
}

GdbStub::Thread::Thread(Process &process, uint64_t thread_id) : process(process), thread_id(thread_id) {
}

GdbStub::Process::Process(uint64_t pid, ITwibDebugger debugger) : pid(pid), debugger(debugger) {
}

GdbStub::Logic::Logic(GdbStub &stub) : stub(stub) {
}

void GdbStub::Logic::Prepare(twibc::SocketServer &server) {
	server.Clear();
	util::Buffer *buffer;
	while((buffer = stub.connection.Process()) != nullptr) {
		LogMessage(Debug, "got message (0x%lx bytes)", buffer->ReadAvailable());
		char ident;
		if(!buffer->Read(ident)) {
			LogMessage(Debug, "invalid packet (zero-length?)");
			stub.connection.SignalError();
			return;
		}
		LogMessage(Debug, "got packet, ident: %c", ident);
		switch(ident) {
		case '!': // extended mode
			stub.connection.RespondOk();
			break;
		case '?': // stop reason
			stub.HandleGetStopReason();
			break;
		case 'H': // set current thread
			stub.HandleSetCurrentThread(*buffer);
			break;
		case 'q': // general get query
			stub.HandleGeneralGetQuery(*buffer);
			break;
		case 'Q': // general set query
			stub.HandleGeneralSetQuery(*buffer);
			break;
		case 'v': // variable
			stub.HandleMultiletterPacket(*buffer);
			break;
		default:
			LogMessage(Info, "unrecognized packet: %c", ident);
			stub.connection.RespondEmpty();
			break;
		}
	}
	server.AddSocket(stub.connection.in_socket);
}

} // namespace gdb
} // namespace twib
} // namespace twili
