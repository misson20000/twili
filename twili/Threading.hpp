//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2019 misson20000 <xenotoad@xenotoad.net>
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
#include<libtransistor/mutex.h>
#include<libtransistor/condvar.h>

namespace twili {
namespace thread {

class Mutex;
class Condvar {
 public:
	Condvar();
	~Condvar();

	void Signal(int n);
	void Wait(Mutex &m, uint64_t timeout);
 private:
	trn_condvar_t condvar;
};

class CAPABILITY("mutex") Mutex {
 public:
	Mutex();
	~Mutex();
	
	void lock() ACQUIRE();
	bool try_lock() TRY_ACQUIRE(true);
	void unlock() RELEASE();

	friend class Condvar;
	
 private:
	trn_mutex_t mutex;
};

class EventThread {
 public:
	EventThread();
	~EventThread();
	void Start();
	void Interrupt();
	void StopAsync();
	void StopSync();
	trn::Waiter event_waiter;
	
 private:
	static void ThreadEntryShim(void *arg);
	void ThreadFunc();

	std::shared_ptr<trn::WaitHandle> interrupt_signal;
	bool exit_requested = false;
	bool thread_running = false;
	trn_thread_t thread;
};

} // namespace thread
} // namespace twili
