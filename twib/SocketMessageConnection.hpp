#pragma once

#include "MessageConnection.hpp"

namespace twili {
namespace twibc {

class SocketMessageConnection : public MessageConnection {
 public:
	class EventThreadNotifier {
	 public:
		virtual void Notify() = 0;
	};
	
	SocketMessageConnection(SOCKET fd, EventThreadNotifier &notifier);
	virtual ~SocketMessageConnection() override;

	bool HasOutData();
	bool PumpOutput(); // returns true if OK
	bool PumpInput(); // returns true if OK

	SOCKET fd;
 protected:
	virtual void SignalInput() override;
	virtual void SignalOutput() override;
 private:
	EventThreadNotifier &notifier;
};

} // namespace twibc
} // namespace twili
