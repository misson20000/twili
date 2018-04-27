#pragma once

#include<libtransistor/cpp/waiter.hpp>
#include<libtransistor/cpp/ipcserver.hpp>

namespace twili {

class Twili {
 public:
	Twili();
	
	bool destroy_flag = false;
	Transistor::Waiter event_waiter;
	Transistor::IPCServer::IPCServer server;
};

}
