#pragma once

#include<libtransistor/cpp/waiter.hpp>

namespace twili {

class TwiliState {
 public:
	bool destroy_server_flag;
	Transistor::Waiter event_waiter;
};

extern TwiliState twili_state;

}
