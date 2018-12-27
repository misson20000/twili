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

#pragma once

#include<optional>
#include<unordered_map>

#include "GdbConnection.hpp"
#include "interfaces/ITwibDeviceInterface.hpp"
#include "interfaces/ITwibDebugger.hpp"

namespace twili {
namespace twib {
namespace gdb {

class GdbStub {
 public:
	GdbStub(ITwibDeviceInterface &itdi);
	~GdbStub();
	
	void Run();
	
	class Query {
	 public:
		Query(GdbStub &stub, std::string field, void (GdbStub::*visitor)(util::Buffer&), bool should_advertise=true);

		const GdbStub &stub;
		const std::string field;
		void (GdbStub::*const visitor)(util::Buffer&);
		const bool should_advertise;
	};

	class Process;
	
	class Thread {
	 public:
		Thread(Process &process, uint64_t thread_id);
		std::vector<uint64_t> GetRegisters();
		Process &process;
		uint64_t thread_id = 0;
	};

	class Process {
	 public:
		Process(uint64_t pid, ITwibDebugger debugger);
		uint64_t pid;
		ITwibDebugger debugger;
		std::map<uint64_t, Thread> threads;
	};
	
	Thread *current_thread = nullptr;
	std::map<uint64_t, Process> attached_processes;
	
	void AddFeature(std::string feature);
	void AddGettableQuery(Query query);
	void AddSettableQuery(Query query);
	void AddMultiletterHandler(std::string name, void (GdbStub::*handler)(util::Buffer&));
	
	std::string stop_reason = "W00";
	
 private:
	ITwibDeviceInterface &itdi;
	GdbConnection connection;
	class Logic : public twibc::SocketServer::Logic {
	 public:
		Logic(GdbStub &stub);
		virtual void Prepare(twibc::SocketServer &server) override;
	 private:
		GdbStub &stub;
	} logic;
	twibc::SocketServer server;

	std::list<std::string> features;
	std::unordered_map<std::string, Query> gettable_queries;
	std::unordered_map<std::string, Query> settable_queries;
	std::unordered_map<std::string, void (GdbStub::*)(util::Buffer&)> multiletter_handlers;

	struct {
		bool valid = false;
		std::map<uint64_t, Process>::iterator process_iterator;
		std::map<uint64_t, Thread>::iterator thread_iterator;
	} get_thread_info;

	// utilities
	void ReadThreadId(util::Buffer &buffer, int64_t &pid, int64_t &thread_id);
	
	// packets
	void HandleGeneralGetQuery(util::Buffer &packet);
	void HandleGeneralSetQuery(util::Buffer &packet);
	void HandleMultiletterPacket(util::Buffer &packet);
	void HandleGetStopReason();
	void HandleDetach(util::Buffer &packet);
	void HandleReadGeneralRegisters();
	void HandleSetCurrentThread(util::Buffer &packet);
	void HandleReadMemory(util::Buffer &packet);
	
	// multiletter packets
	void HandleVAttach(util::Buffer &packet);
	void HandleVContQuery(util::Buffer &packet);
	void HandleVCont(util::Buffer &packet);
	
	// get queries
	void QueryGetSupported(util::Buffer &packet);
	void QueryGetCurrentThread(util::Buffer &packet);
	void QueryGetFThreadInfo(util::Buffer &packet);
	void QueryGetSThreadInfo(util::Buffer &packet);
	void QueryGetThreadExtraInfo(util::Buffer &packet);
	
	// set queries
	void QuerySetStartNoAckMode(util::Buffer &packet);

	void ProcessEvents(Process &process);
};

} // namespace gdb
} // namespace twib
} // namespace twili
