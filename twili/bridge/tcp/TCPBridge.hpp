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

#include<libtransistor/cpp/waiter.hpp>
#include<libtransistor/thread.h>
#include<libtransistor/condvar.h>
#include<libtransistor/mutex.h>

#include<list>
#include<memory>

#include "../../../common/Protocol.hpp"
#include "../../../common/Buffer.hpp"
#include "../ResponseOpener.hpp"
#include "../RequestHandler.hpp"

#include "../../ipcbind/nifm/IRequest.hpp"
#include "../../Socket.hpp"

namespace twili {

class Twili;

namespace bridge {

class Object;

namespace tcp {

class TCPBridge {
 public:
	class Connection;
	
	TCPBridge(Twili &twili, std::shared_ptr<bridge::Object> object_zero);
	~TCPBridge();

	TCPBridge(const TCPBridge&) = delete;
	TCPBridge &operator=(TCPBridge const&) = delete;
	
 private:
	Twili &twili;

	util::Socket announce_socket;
	util::Socket server_socket;
	std::list<std::shared_ptr<Connection>> connections;
	std::shared_ptr<bridge::Object> object_zero;
	
	service::nifm::IRequest network;
	
	bool thread_destroy = false;
	trn_thread_t thread;
	static void ThreadEntryShim(void *arg);
	void SocketThread();

	void ResetSockets();
	service::nifm::IRequest::State network_state = service::nifm::IRequest::State::Error;
	trn::KEvent network_state_event;
	trn_mutex_t network_state_mutex = TRN_MUTEX_STATIC_INITIALIZER;
	trn_condvar_t network_state_condvar = TRN_CONDVAR_STATIC_INITIALIZER;
	std::shared_ptr<trn::WaitHandle> network_state_wh;

	trn_mutex_t request_processing_mutex = TRN_MUTEX_STATIC_INITIALIZER;
	trn_condvar_t request_processing_condvar = TRN_CONDVAR_STATIC_INITIALIZER;
	std::shared_ptr<trn::WaitHandle> request_processing_signal_wh;
	std::shared_ptr<Connection> request_processing_connection;
};

class TCPBridge::Connection : public std::enable_shared_from_this<TCPBridge::Connection> {
 public:
	class ResponseState;
	
	Connection(TCPBridge &bridge, util::Socket &&socket);

	void PumpInput();
	void Process();

	// called when command processing has ended and further input should be discarded
	void ResetHandler();
	
	bool deletion_flag = false;

	enum class Task {
		Idle, BeginProcessingCommand, FlushReceiveBuffer, FinalizeCommand
	};
	
	volatile Task pending_task = Task::Idle;

	void Synchronized(); // called on main thread
	
	util::Socket socket;
 private:
	TCPBridge &bridge;

	/*
	 * Requests that the main thread process something,
	 * and then blocks until the main thread finishes. I know that this
	 * isn't very parallel, but I'm only using threads here to work around
	 * bad socket synchronization primitives so I don't really care.
	 * The faster we can block this (the I/O) thread and keep it from breaking
	 * things, the better.
	 */
	// TODO: use a mutex here and run things on this thread to simplify
	//       control flow.
	void Synchronize(Task task);
	
	void BeginProcessingCommandImpl(); // should run on main thread
	
	util::Buffer in_buffer;

	bool has_current_mh = false;
	bool has_current_payload = false;
	protocol::MessageHeader current_mh;
	size_t payload_size;
	util::Buffer payload_buffer;
	util::Buffer current_object_ids;

	std::shared_ptr<detail::ResponseState> current_state;
	RequestHandler &current_handler = DiscardingRequestHandler::GetInstance();
	
	uint32_t next_object_id = 1;
	std::map<uint32_t, std::shared_ptr<bridge::Object>> objects;
};

class TCPBridge::Connection::ResponseState : public bridge::detail::ResponseState {
 public:
	ResponseState(std::shared_ptr<Connection> connection, uint32_t client_id, uint32_t tag);
	
	virtual size_t GetMaxTransferSize() override;
	virtual void SendHeader(protocol::MessageHeader &hdr) override;
	virtual void SendData(uint8_t *data, size_t size) override;
	virtual void Finalize() override;
	virtual uint32_t ReserveObjectId() override;
	virtual void InsertObject(std::pair<uint32_t, std::shared_ptr<Object>> &&pair) override;
	
 private:
	void Send(uint8_t *data, size_t size);
	std::shared_ptr<Connection> connection;
};

} // namespace tcp
} // namespace bridge
} // namespace twili
