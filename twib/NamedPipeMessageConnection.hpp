#pragma once

#include "platform.hpp"
#include "MessageConnection.hpp"
#include "EventThreadNotifier.hpp"
#include "NamedPipeServer.hpp"

namespace twili {
namespace twibc {

class NamedPipeMessageConnection : public MessageConnection {
 public:
	NamedPipeMessageConnection(platform::windows::Pipe &&pipe, const EventThreadNotifier &notifier);
	NamedPipeMessageConnection(NamedPipeServer::Pipe &&pipe, const EventThreadNotifier &notifier);
	virtual ~NamedPipeMessageConnection() override;

	void Signal();

	class MessagePipe : public NamedPipeServer::Pipe {
	public:
		MessagePipe(NamedPipeMessageConnection &connection, platform::windows::Pipe &&pipe);
		MessagePipe(NamedPipeMessageConnection &connection, NamedPipeServer::Pipe &&other);

		virtual bool WantsSignalIn() override;
		virtual bool WantsSignalOut() override;
		virtual void SignalIn() override;
		virtual void SignalOut() override;
	private:
		NamedPipeMessageConnection &connection;
	} pipe;
 protected:
	virtual bool RequestInput() override;
	virtual bool RequestOutput() override;
 private:
	bool is_reading = false;
	bool is_writing = false;
	std::mutex state_mutex;
	std::unique_lock<std::recursive_mutex> out_buffer_lock;
	const EventThreadNotifier &notifier;
};

} // namespace twibc
} // namespace twili
