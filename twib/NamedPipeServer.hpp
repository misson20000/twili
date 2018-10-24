#pragma once

#include "platform.hpp"

#include<list>
#include<vector>
#include<thread>
#include<mutex>

#include<stdint.h>

#include "EventThreadNotifier.hpp"

namespace twili {
namespace twibc {

class NamedPipeServer {
 public:
	class Logic {
	 public:
		virtual void Prepare(NamedPipeServer &server) = 0;
	};
	
	NamedPipeServer(Logic &logic);
	~NamedPipeServer();

	class Pipe {
	 public:
		 Pipe();
		 Pipe(platform::windows::Pipe &&pipe);
		 Pipe(Pipe &&other);
		 ~Pipe();

		 Pipe &operator=(Pipe &&other);

		 bool IsValid();
		 void Close();

		 virtual bool WantsSignalIn();
		 virtual bool WantsSignalOut();
		 virtual void SignalIn();
		 virtual void SignalOut();

		 platform::windows::Pipe pipe;
		 platform::windows::Event event_in;
		 platform::windows::Event event_out;
		 OVERLAPPED overlap_in = {0};
		 OVERLAPPED overlap_out = {0};

		 size_t last_service = 0;
	};

	void Begin();
	void Destroy();
	void Clear();
	void AddPipe(Pipe &pipe);

	class EventThreadNotifier : public twibc::EventThreadNotifier {
	 public:
		EventThreadNotifier(NamedPipeServer &server);
		virtual void Notify() const override;
	 private:
		NamedPipeServer &server;
	} const notifier;
	
 private:
	std::vector<std::reference_wrapper<Pipe>> pipes;
	Logic &logic;
	
	bool event_thread_destroy = false;
	void event_thread_func();
	std::thread event_thread;

	platform::windows::Event notification_event;
};

} // namespace twibc
} // namespace twili
