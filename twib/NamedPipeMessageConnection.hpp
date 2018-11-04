#pragma once

#include "platform.hpp"
#include "MessageConnection.hpp"
#include "EventThreadNotifier.hpp"
#include "platform/windows/EventLoop.hpp"

namespace twili {
namespace twibc {

class NamedPipeMessageConnection : public MessageConnection {
 public:
	NamedPipeMessageConnection(platform::windows::Pipe &&pipe, const EventThreadNotifier &notifier);
	virtual ~NamedPipeMessageConnection() override;

	class InputMember : public platform::windows::EventLoop::Member {
	public:
		InputMember(NamedPipeMessageConnection &connection);

		virtual bool WantsSignal() override;
		virtual void Signal() override;
		virtual platform::windows::Event &GetEvent() override;

		OVERLAPPED overlap = { 0 };
	private:
		NamedPipeMessageConnection &connection;
		platform::windows::Event event;
	} input_member;

	class OutputMember : public platform::windows::EventLoop::Member {
	public:
		OutputMember(NamedPipeMessageConnection &connection);

		virtual bool WantsSignal() override;
		virtual void Signal() override;
		virtual platform::windows::Event &GetEvent() override;

		OVERLAPPED overlap = { 0 };
	private:
		NamedPipeMessageConnection &connection;
		platform::windows::Event event;
	} output_member;

 protected:
	virtual bool RequestInput() override;
	virtual bool RequestOutput() override;
 private:
	bool is_reading = false;
	bool is_writing = false;
	std::mutex state_mutex;
	std::unique_lock<std::recursive_mutex> out_buffer_lock;
	platform::windows::Pipe pipe;
	const EventThreadNotifier &notifier;
};

} // namespace twibc
} // namespace twili
