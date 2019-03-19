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

#include "common/Logger.hpp"
#include "common/ResultError.hpp"

namespace twili {
namespace twib {
namespace tool {
namespace gdb {

GdbStub::GdbStub(ITwibDeviceInterface &itdi) :
	itdi(itdi),
	connection(
		platform::File(STDIN_FILENO, false),
		platform::File(STDOUT_FILENO, false)),
	logic(*this),
	loop(logic) {
	AddGettableQuery(Query(*this, "Supported", &GdbStub::QueryGetSupported, false));
	AddGettableQuery(Query(*this, "C", &GdbStub::QueryGetCurrentThread, false));
	AddGettableQuery(Query(*this, "fThreadInfo", &GdbStub::QueryGetFThreadInfo, false));
	AddGettableQuery(Query(*this, "sThreadInfo", &GdbStub::QueryGetSThreadInfo, false));
	AddGettableQuery(Query(*this, "ThreadExtraInfo", &GdbStub::QueryGetThreadExtraInfo, false, ','));
	AddGettableQuery(Query(*this, "Offsets", &GdbStub::QueryGetOffsets, false));
	AddGettableQuery(Query(*this, "Rcmd", &GdbStub::QueryGetRemoteCommand, false, ','));
	AddSettableQuery(Query(*this, "StartNoAckMode", &GdbStub::QuerySetStartNoAckMode));
	AddMultiletterHandler("Attach", &GdbStub::HandleVAttach);
	AddMultiletterHandler("Cont?", &GdbStub::HandleVContQuery);
	AddMultiletterHandler("Cont", &GdbStub::HandleVCont);
}

GdbStub::~GdbStub() {
	loop.Destroy();
}

void GdbStub::Run() {
	std::unique_lock<std::mutex> lock(connection.mutex);
	loop.Begin();
	while(!connection.error_flag) {
		connection.error_condvar.wait(lock);
	}
}

GdbStub::Query::Query(GdbStub &stub, std::string field, void (GdbStub::*visitor)(util::Buffer&), bool should_advertise, char separator) :
	stub(stub),
	field(field),
	visitor(visitor),
	should_advertise(should_advertise),
	separator(separator) {
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

void GdbStub::Stop() {
	waiting_for_stop = false;
	HandleGetStopReason(); // send reason
}

void GdbStub::ReadThreadId(util::Buffer &packet, int64_t &pid, int64_t &thread_id) {
	pid = current_thread ? current_thread->process.pid : 0;
	
	char ch;
	if(packet.ReadAvailable() && packet.Read()[0] == 'p') { // peek
		packet.MarkRead(1); // consume
		if(packet.ReadAvailable() && packet.Read()[0] == '-') { // all processes
			pid = -1;
			packet.MarkRead(1); // consume
		} else {
			uint64_t dec_pid;
			GdbConnection::DecodeWithSeparator(dec_pid, '.', packet);
			pid = dec_pid;
		}
	}

	if(packet.ReadAvailable() && packet.Read()[0] == '-') { // all threads
		thread_id = -1;
		packet.MarkRead(1); // consume
	} else {
		uint64_t dec_thread_id;
		GdbConnection::Decode(dec_thread_id, packet);
		thread_id = dec_thread_id;
	}
}

void GdbStub::HandleGeneralGetQuery(util::Buffer &packet) {
	std::string field;
	char ch;
	std::unordered_map<std::string, Query>::iterator i = gettable_queries.end();
	while(packet.Read(ch)) {
		if(i != gettable_queries.end() && ch == i->second.separator) {
			break;
		}
		field.push_back(ch);
		i = gettable_queries.find(field);
	}
	LogMessage(Debug, "got get query for '%s'", field.c_str());

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
	std::unordered_map<std::string, Query>::iterator i = settable_queries.end();
	while(packet.Read(ch)) {
		if(i != settable_queries.end() && ch == i->second.separator) {
			break;
		}
		field.push_back(ch);
		i = settable_queries.find(field);
	}
	LogMessage(Debug, "got set query for '%s'", field.c_str());

	if(i != settable_queries.end()) {
		std::invoke(i->second.visitor, this, packet);
	} else {
		LogMessage(Info, "unsupported query: '%s'", field.c_str());
		connection.RespondEmpty();
	}
}

void GdbStub::HandleIsThreadAlive(util::Buffer &packet) {
	connection.RespondOk();
}

void GdbStub::HandleMultiletterPacket(util::Buffer &packet) {
	std::string title;
	char ch;
	while(packet.Read(ch) && ch != ';') {
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

void GdbStub::HandleDetach(util::Buffer &packet) {
	char ch;
	if(packet.Read(ch)) {
		if(ch != ';') {
			LogMessage(Warning, "invalid detach packet");
			connection.RespondError(1);
			return;
		}
		
		uint64_t pid;
		GdbConnection::Decode(pid, packet);
		LogMessage(Debug, "detaching from 0x%x", pid);
		if(current_thread->process.pid == pid) {
			current_thread = nullptr;
		}
		get_thread_info.valid = false;
		attached_processes.erase(pid);
	} else { // detach all
		LogMessage(Debug, "detaching from all");
		current_thread = nullptr;
		get_thread_info.valid = false;
		attached_processes.clear();
	}
	stop_reason = "W00";
	connection.RespondOk();
}

void GdbStub::HandleReadGeneralRegisters() {
	if(current_thread == nullptr) {
		LogMessage(Warning, "attempt to read registers with no selected thread");
		connection.RespondError(1);
		return;
	}

	try {
		util::Buffer response;
		std::vector<uint64_t> registers = current_thread->GetRegisters();
		GdbConnection::Encode((uint8_t*) registers.data(), 284, response);
		std::string str;
		size_t sz = response.ReadAvailable();
		response.Read(str, sz);
		response.MarkRead(-sz);
		LogMessage(Debug, "responding with '%s'", str.c_str());
		connection.Respond(response);
	} catch(ResultError &e) {
		LogMessage(Debug, "failed to read registers: 0x%x", e.code);
		connection.RespondError(e.code);
	}
}

void GdbStub::HandleWriteGeneralRegisters(util::Buffer &packet) {
	if(current_thread == nullptr) {
		LogMessage(Warning, "attempt to write registers with no selected thread");
		connection.RespondError(1);
	}

	std::vector<uint8_t> registers_binary;
	GdbConnection::Decode(registers_binary, packet);

	std::vector<uint64_t> registers(36);
	memcpy(registers.data(), registers_binary.data(), 284);
	
	try {
		current_thread->SetRegisters(registers);
		connection.RespondOk();
	} catch(ResultError &e) {
		LogMessage(Debug, "failed to write registers: 0x%x", e.code);
		connection.RespondError(e.code);
	}
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

	int64_t pid, thread_id;
	ReadThreadId(packet, pid, thread_id);

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

void GdbStub::HandleReadMemory(util::Buffer &packet) {
	uint64_t address, size;
	GdbConnection::DecodeWithSeparator(address, ',', packet);
	GdbConnection::Decode(size, packet);

	if(!current_thread) {
		LogMessage(Warning, "attempted to read without selected thread");
		connection.RespondError(1);
	}

	LogMessage(Debug, "reading 0x%lx bytes from 0x%lx", size, address);

	try {
		util::Buffer response;
		std::vector<uint8_t> mem = current_thread->process.debugger.ReadMemory(address, size);
		GdbConnection::Encode(mem.data(), mem.size(), response);
		connection.Respond(response);
	} catch(ResultError &e) {
		connection.RespondError(e.code);
	}
}

void GdbStub::HandleWriteMemory(util::Buffer &packet) {
	uint64_t address, size;
	GdbConnection::DecodeWithSeparator(address, ',', packet);
	GdbConnection::DecodeWithSeparator(size, ':', packet);

	if(!current_thread) {
		LogMessage(Warning, "attempted to write without selected thread");
		connection.RespondError(1);
	}

	LogMessage(Debug, "writing 0x%lx bytes to 0x%lx", size, address);

	std::vector<uint8_t> bytes;
	GdbConnection::Decode(bytes, packet);
	if(bytes.size() != size) {
		LogMessage(Error, "size mismatch (0x%lx != 0x%lx)", bytes.size(), size);
		connection.RespondError(1);
		return;
	}

	LogMessage(Debug, "  %lx", *(uint32_t*) bytes.data());
	
	try {
		util::Buffer response;
		current_thread->process.debugger.WriteMemory(address, bytes);
		connection.RespondOk();
	} catch(ResultError &e) {
		connection.RespondError(e.code);
	}
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
	
	r.first->second.IngestEvents(*this);

	if(r.first->second.threads.empty()) {
		// try to start it with pm:dmnt
		try {
			r.first->second.debugger.LaunchDebugProcess();
		} catch(ResultError &e) {
			LogMessage(Warning, "attached to process with no threads, and LaunchDebugProcess failed: 0x%lx", e.code);
		}
		r.first->second.IngestEvents(*this);
	}
	
	// ok
	HandleGetStopReason();
}

void GdbStub::HandleVContQuery(util::Buffer &packet) {
	util::Buffer response;
	response.Write(std::string("vCont;c;C"));
	connection.Respond(response);
}

void GdbStub::HandleVCont(util::Buffer &packet) {
	char ch;
	util::Buffer action_buffer;
	util::Buffer thread_id_buffer;
	bool reading_action = true;
	bool read_success = true;

	struct Action {
		enum class Type {
			Invalid,
			Continue
		} type = Type::Invalid;
	};

	std::map<uint64_t, std::map<uint64_t, Action>> process_actions;
	
	while(read_success) {
		read_success = packet.Read(ch);
		if(!read_success || ch == ';') {
			if(!action_buffer.Read(ch)) {
				LogMessage(Warning, "invalid vCont action: too small");
				connection.RespondError(1);
				return;
			}
			
			Action action;
			switch(ch) {
			case 'C':
				LogMessage(Warning, "vCont 'C' action not well supported");
				// fall-through
			case 'c':
				action.type = Action::Type::Continue;
				break;
			default:
				LogMessage(Warning, "unsupported vCont action: %c", ch);
			}

			if(action.type != Action::Type::Invalid) {
				int64_t pid = -1;
				int64_t thread_id = -1;
				if(thread_id_buffer.ReadAvailable()) {
					ReadThreadId(thread_id_buffer, pid, thread_id);
				}
				LogMessage(Debug, "vCont %ld, %ld action %c\n", pid, thread_id, ch);
				if(pid == -1) {
					for(auto &p : attached_processes) {
						std::map<uint64_t, Action> &thread_actions = process_actions.insert({p.first, {}}).first->second;
						for(auto &t : p.second.threads) {
							thread_actions.insert({t.first, action});
						}
					}
				} else {
					auto p = attached_processes.find(pid);
					if(p != attached_processes.end()) {
						std::map<uint64_t, Action> &thread_actions = process_actions.insert({p->first, {}}).first->second;
						if(thread_id == -1) {
							for(auto &t : p->second.threads) {
								thread_actions.insert({t.first, action});
							}
						} else {
							auto t = p->second.threads.find(thread_id);
							if(t != p->second.threads.end()) {
								thread_actions.insert({t->first, action});
							}
						}
					}
				}
			}
			
			reading_action = true;
			action_buffer.Clear();
			thread_id_buffer.Clear();
			continue;
		}
		if(ch == ':') {
			reading_action = false;
			continue;
		}
		if(reading_action) {
			action_buffer.Write(ch);
		} else {
			thread_id_buffer.Write(ch);
		}
	}

	for(auto p : process_actions) {
		auto p_i = attached_processes.find(p.first);
		if(p_i == attached_processes.end()) {
			LogMessage(Warning, "no such process: 0x%lx", p.first);
			continue;
		}
		Process &proc = p_i->second;
		LogMessage(Debug, "ingesting process events before continue...");
		if(proc.IngestEvents(*this)) {
			LogMessage(Debug, "  stopped");
			Stop();
			return;
		}
		std::vector<uint64_t> thread_ids;
		for(auto &t : p.second) {
			auto t_i = proc.threads.find(t.first);
			if(t_i == proc.threads.end()) {
				LogMessage(Warning, "no such thread: 0x%lx", t.first);
				continue;
			}
			thread_ids.push_back(t.first);
		}
		LogMessage(Debug, "continuing process");
		for(auto &t : thread_ids) {
			LogMessage(Debug, "  tid 0x%lx", t);
		}
		proc.debugger.ContinueDebugEvent(7, thread_ids);
		proc.running = true;
	}
	waiting_for_stop = true;
	LogMessage(Debug, "reached end of vCont");
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

void GdbStub::QueryGetFThreadInfo(util::Buffer &packet) {
	get_thread_info.process_iterator = attached_processes.begin();
	get_thread_info.thread_iterator = get_thread_info.process_iterator->second.threads.begin();
	get_thread_info.valid = true;
	QueryGetSThreadInfo(packet);
}

void GdbStub::QueryGetSThreadInfo(util::Buffer &packet) {
	util::Buffer response;
	if(!get_thread_info.valid) {
		LogMessage(Warning, "get_thread_info iterators invalidated");
		connection.RespondError(1);
		return;
	}
	
	bool has_written = false;
	for(;
			get_thread_info.process_iterator != attached_processes.end();
			get_thread_info.process_iterator++,
				get_thread_info.thread_iterator = get_thread_info.process_iterator->second.threads.begin()) {
		for(;
				get_thread_info.thread_iterator != get_thread_info.process_iterator->second.threads.end();
				get_thread_info.thread_iterator++) {
			Thread &thread = get_thread_info.thread_iterator->second;
			if(has_written) {
				response.Write(',');
			} else {
				response.Write('m');
			}
			response.Write('p');
			GdbConnection::Encode(thread.process.pid, 0, response);
			response.Write('.');
			GdbConnection::Encode(thread.thread_id, 0, response);
			has_written = true;
		}
	}
	if(!has_written) {
		get_thread_info.valid = false;
		response.Write('l'); // end of list
	}
	connection.Respond(response);
}

void GdbStub::QueryGetThreadExtraInfo(util::Buffer &packet) {
	int64_t pid, thread_id;
	ReadThreadId(packet, pid, thread_id);

	auto i = attached_processes.find(pid);
	if(i == attached_processes.end()) {
		LogMessage(Debug, "no such process with pid 0x%x", pid);
		connection.RespondError(1);
		return;
	}

	Process &p = i->second;
	
	auto j = p.threads.find(thread_id);
	if(j == p.threads.end()) {
		LogMessage(Debug, "no such thread with tid 0x%x", thread_id);
		connection.RespondError(1);
		return;
	}

	Thread &t = j->second;

	std::string extra_info;

	try {
		std::vector<uint8_t> tls_ctx_ptr_u8 = p.debugger.ReadMemory(t.tls_addr + 0x1f8, 8);
		uint64_t tls_ctx_addr = *(uint64_t*) tls_ctx_ptr_u8.data();
		std::vector<uint8_t> name_ptr_u8 = p.debugger.ReadMemory(tls_ctx_addr + 0x1a8, 8);
		uint64_t name_addr = *(uint64_t*) name_ptr_u8.data();
	
		if(name_addr != 0) {
			std::vector<uint8_t> name = p.debugger.ReadMemory(name_addr, 0x20);
			extra_info = (char*) name.data();
		}
	} catch(ResultError &e) {
	}
	
	util::Buffer response;
	GdbConnection::Encode(extra_info, response);
	connection.Respond(response);
}

void GdbStub::QueryGetOffsets(util::Buffer &packet) {
	if(current_thread == nullptr) {
		connection.RespondError(1);
		return;
	}
	
	uint64_t addr = current_thread->process.debugger.GetTargetEntry();
	if(addr == 0) {
		nx::MemoryInfo mi;
		while((mi = std::get<0>(current_thread->process.debugger.QueryMemory(addr))).memory_type != 3) {
			if(mi.base_addr + mi.size < addr) {
				connection.RespondError(2);
				return;
			}
			addr = mi.base_addr + mi.size;
		}

		LogMessage(Debug, "found aslr base at 0x%lx", addr);
	}
	
	util::Buffer response;
	response.Write("TextSeg=");
	GdbConnection::Encode(addr, 8, response);
	connection.Respond(response);
}

void GdbStub::QueryGetRemoteCommand(util::Buffer &packet) {
	util::Buffer message;
	GdbConnection::Decode(message, packet);
	
	std::string command;
	char ch;
	while(message.Read(ch) && ch != ' ') {
		command.push_back(ch);
	}

	std::stringstream response;
	try {
		if(command == "help") {
			response << "Available commands:" << std::endl;
			response << "  - wait application" << std::endl;
			response << "  - wait title <title id>" << std::endl;
		} else if(command == "wait") {
			std::string wait_for;
			while(message.Read(ch) && ch != ' ') {
				wait_for.push_back(ch);
			}
			if(wait_for == "application") {
				if(message.ReadAvailable()) {
					response << "Syntax error: expected end of input" << std::endl;
				} else {
					uint64_t pid = itdi.WaitToDebugApplication();
					response << "PID: 0x" << std::hex << pid << std::endl;
				}
			} else {
				std::string tid_str;
				while(message.Read(ch) && ch != ' ') {
					tid_str.push_back(ch);
				}
				if(message.ReadAvailable()) {
					response << "Syntax error: expected end of input" << std::endl;
				} else {
					uint64_t tid = std::stoull(tid_str, 0, 16);
					uint64_t pid = itdi.WaitToDebugTitle(tid);
					response << "PID: 0x" << std::hex << pid << std::endl;
				}
			}
		} else {
			response << "Unknown command '" << command << "'" << std::endl;
		}
	} catch(ResultError &e) {
		response = std::stringstream();
		response << "Target error: 0x" << std::hex << e.code << std::endl;
	} catch(std::invalid_argument &e) {
		response = std::stringstream();
		response << "Invalid argument." << std::endl;
	}
	util::Buffer response_buffer;
	GdbConnection::Encode(response.str(), response_buffer);
	connection.Respond(response_buffer);
}

void GdbStub::QuerySetStartNoAckMode(util::Buffer &packet) {
	connection.StartNoAckMode();
	connection.RespondOk();
}

bool GdbStub::Process::IngestEvents(GdbStub &stub) {
	std::optional<nx::DebugEvent> event;

	int signal = 0;
	bool stopped = false;
	
	while(event = debugger.GetDebugEvent()) {
		LogMessage(Debug, "got event: %d", event->event_type);

		stopped = true;
		
		if(event->thread_id) {
			auto t_i = threads.find(event->thread_id);
			if(t_i != threads.end()) {
				stub.current_thread = &t_i->second;
			}
		}
		
		switch(event->event_type) {
		case nx::DebugEvent::EventType::AttachProcess: {
			break; }
		case nx::DebugEvent::EventType::AttachThread: {
			uint64_t thread_id = event->attach_thread.thread_id;
			LogMessage(Debug, "  attaching new thread: 0x%x", thread_id);
			auto r = threads.emplace(thread_id, Thread(*this, thread_id, event->attach_thread.tls_pointer));
			stub.current_thread = &r.first->second;
			break; }
		case nx::DebugEvent::EventType::ExitProcess: {
			LogMessage(Warning, "process exited");
			break; }
		case nx::DebugEvent::EventType::ExitThread: {
			LogMessage(Warning, "thread exited");
			break; }
		case nx::DebugEvent::EventType::Exception: {
			LogMessage(Warning, "hit exception");
			running = false;
			switch(event->exception.exception_type) {
			case nx::DebugEvent::ExceptionType::Trap:
				LogMessage(Warning, "trap");
				signal = 5; // SIGTRAP
				break;
			case nx::DebugEvent::ExceptionType::InstructionAbort:
				LogMessage(Warning, "instruction abort");
				signal = 145; // EXC_BAD_ACCESS
				break;
			case nx::DebugEvent::ExceptionType::DataAbortMisc:
				LogMessage(Warning, "data abort misc");
				signal = 11; // SIGSEGV
				break;
			case nx::DebugEvent::ExceptionType::PcSpAlignmentFault:
				LogMessage(Warning, "pc sp alignment fault");
				signal = 145; // EXC_BAD_ACCESS
				break;
			case nx::DebugEvent::ExceptionType::DebuggerAttached:
				LogMessage(Warning, "debugger attached");
				signal = 0; // no signal
				break;
			case nx::DebugEvent::ExceptionType::BreakPoint:
				LogMessage(Warning, "breakpoint");
				signal = 5; // SIGTRAP
				break;
			case nx::DebugEvent::ExceptionType::UserBreak:
				LogMessage(Warning, "user break");
				signal = 149; // EXC_SOFTWARE
				break;
			case nx::DebugEvent::ExceptionType::DebuggerBreak:
				LogMessage(Warning, "debugger break");
				signal = 2; // SIGINT
				break;
			case nx::DebugEvent::ExceptionType::BadSvcId:
				LogMessage(Warning, "bad svc id");
				signal = 12; // SIGSYS
				break;
			case nx::DebugEvent::ExceptionType::SError:
				LogMessage(Warning, "SError");
				signal = 10; // SIGBUS
				break;
			}
			break; }
		}
	}

	if(stopped) {
		util::Buffer stop_reason;
		stop_reason.Write('T');
		GdbConnection::Encode(signal, 1, stop_reason);
		stub.stop_reason = stop_reason.GetString();
	}

	if(!stub.has_async_wait) {
		std::shared_ptr<bool> has_events = this->has_events;
		debugger.AsyncWait(
			[has_events, &stub](uint32_t r) {
				stub.has_async_wait = false;
				if(r == 0) {
					LogMessage(Debug, "process got event signal");
					*has_events = true;
					stub.loop.GetNotifier().Notify();
				} else {
					LogMessage(Error, "process got error signal");
				}
			});
		stub.has_async_wait = true;
	}

	return stopped;
}

GdbStub::Thread::Thread(Process &process, uint64_t thread_id, uint64_t tls_addr) : process(process), thread_id(thread_id), tls_addr(tls_addr) {
}

std::vector<uint64_t> GdbStub::Thread::GetRegisters() {
	return process.debugger.GetThreadContext(thread_id);
}

void GdbStub::Thread::SetRegisters(std::vector<uint64_t> registers) {
	return process.debugger.SetThreadContext(thread_id, registers);
}

GdbStub::Process::Process(uint64_t pid, ITwibDebugger debugger) : pid(pid), debugger(debugger) {
	has_events = std::make_shared<bool>(false);
}

GdbStub::Logic::Logic(GdbStub &stub) : stub(stub) {
}

void GdbStub::Logic::Prepare(platform::EventLoop &loop) {
	loop.Clear();
	util::Buffer *buffer;
	bool interrupted;
	while((buffer = stub.connection.Process(interrupted)) != nullptr) {
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
		case 'D': // detach
			stub.HandleDetach(*buffer);
			break;
		case 'g': // read general registers
			stub.HandleReadGeneralRegisters();
			break;
		case 'G': // write general registers
			stub.HandleWriteGeneralRegisters(*buffer);
			break;
		case 'H': // set current thread
			stub.HandleSetCurrentThread(*buffer);
			break;
		case 'm': // read memory
			stub.HandleReadMemory(*buffer);
			break;
		case 'M': // write memory
			stub.HandleWriteMemory(*buffer);
			break;
		case 'q': // general get query
			stub.HandleGeneralGetQuery(*buffer);
			break;
		case 'Q': // general set query
			stub.HandleGeneralSetQuery(*buffer);
			break;
		case 'T': // is thread alive
			stub.HandleIsThreadAlive(*buffer);
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

	if(interrupted || stub.waiting_for_stop) {
		for(auto &p : stub.attached_processes) {
			if(interrupted && p.second.running) {
				p.second.debugger.BreakProcess();
			}
			if(stub.waiting_for_stop && *p.second.has_events) {
				if(p.second.IngestEvents(stub)) {
					LogMessage(Debug, "stopping due to received event");
					stub.Stop();
				}
			}
		}
	}
	
	loop.AddMember(stub.connection.in_member);
}

} // namespace gdb
} // namespace tool
} // namespace twib
} // namespace twili
