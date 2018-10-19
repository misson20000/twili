#pragma once

#include "platform.hpp"

#include<Windows.h>

#include<list>
#include<vector>
#include<thread>
#include<mutex>

#include<stdint.h>

#include "platform/windows.hpp"
#include "Frontend.hpp"
#include "Messages.hpp"
#include "Protocol.hpp"
#include "Buffer.hpp"
#include "MessageConnection.hpp"

namespace twili {
namespace twibd {

class Twibd;

namespace frontend {

class NamedPipeFrontend : public Frontend {
public:
	NamedPipeFrontend(Twibd *twibd, const char *name);
	~NamedPipeFrontend();

	class Client : public twibd::Client {
	public:
		enum class State {
			Connecting,
			Reading,
			Processing,
			Writing,
			Error,
		};

		Client(NamedPipeFrontend &frontend);
		~Client();

		void IncomingMessage(protocol::MessageHeader &mh, util::Buffer &payload, util::Buffer &object_ids);
		virtual void PostResponse(Response &r);

		void Signal();
		void Read();
		void Write();
		void Process();
		void Error();

		std::mutex out_buffer_mutex;
		util::Buffer out_buffer;

		bool has_current_mh = false;
		bool has_current_payload = false;
		protocol::MessageHeader current_mh;
		util::Buffer current_payload;
		util::Buffer current_object_ids;

		util::Buffer in_buffer;
		platform::windows::Pipe pipe;
		platform::windows::Event event;
		State state = State::Connecting;
		OVERLAPPED overlap;

		NamedPipeFrontend &frontend;
		Twibd *twibd;
	};

private:
	Twibd *twibd;

	void ClientConnected(std::shared_ptr<twibd::Client> client);

	bool event_thread_destroy = false;
	void event_thread_func();
	std::thread event_thread;

	std::shared_ptr<Client> pending_client;
	std::list<std::shared_ptr<Client>> connections;
};

} // namespace frontend
} // namespace twibd
} // namespace twili
