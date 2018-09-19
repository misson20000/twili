#pragma once

#include<libtransistor/cpp/waiter.hpp>
#include<libtransistor/thread.h>
#include<libtransistor/condvar.h>
#include<libtransistor/mutex.h>

#include<list>
#include<memory>

#include "twib/Protocol.hpp"
#include "twib/Buffer.hpp"
#include "bridge/ResponseOpener.hpp"
#include "service/nifm/IRequest.hpp"
#include "Socket.hpp"

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
};

class TCPBridge::Connection : public std::enable_shared_from_this<TCPBridge::Connection> {
 public:
	class ResponseState;
	
	Connection(TCPBridge &bridge, util::Socket &&socket);

	void PumpInput();
	void Process();
	void ProcessCommand();
	bool deletion_flag = false;

	util::Socket socket;
 private:
	TCPBridge &bridge;

	util::Buffer in_buffer;

	bool has_current_mh = false;
	bool has_current_payload = false;
	protocol::MessageHeader current_mh;
	util::Buffer current_payload;
	util::Buffer current_object_ids;
	
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
