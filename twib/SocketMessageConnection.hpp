#pragma once

#include "MessageConnection.hpp"
#include "SocketServer.hpp"
#include "EventThreadNotifier.hpp"

namespace twili {
namespace twibc {

class SocketMessageConnection : public MessageConnection {
 public:
	SocketMessageConnection(SOCKET fd, const EventThreadNotifier &notifier);
	virtual ~SocketMessageConnection() override;

	class MessageSocket : public SocketServer::Socket {
	 public:
		MessageSocket(SocketMessageConnection &connection, SOCKET fd);

		virtual bool WantsRead() override;
		virtual bool WantsWrite() override;
		virtual void SignalRead() override;
		virtual void SignalWrite() override;
		virtual void SignalError() override;
	 private:
		SocketMessageConnection &connection;
	} socket;

 protected:
	virtual void RequestInput() override;
	virtual void RequestOutput() override;
 private:
	const EventThreadNotifier &notifier;
};

} // namespace twibc
} // namespace twili
