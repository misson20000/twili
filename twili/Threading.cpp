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

#include "Threading.hpp"

namespace twili {
namespace thread {

using namespace trn;

Mutex::Mutex() {
	trn_mutex_create(&mutex);
}

Mutex::~Mutex() {
}

void Mutex::lock() {
	trn_mutex_lock(&mutex);
}

bool Mutex::try_lock() {
	return trn_mutex_try_lock(&mutex);
}

void Mutex::unlock() {
	trn_mutex_unlock(&mutex);
}

Condvar::Condvar() {
	trn_condvar_create(&condvar);
}

Condvar::~Condvar() {
	trn_condvar_destroy(&condvar);
}

void Condvar::Signal(int n) {
	trn_condvar_signal(&condvar, n);
}

void Condvar::Wait(Mutex &m, uint64_t timeout) {
	trn_condvar_wait(&condvar, &m.mutex, timeout);
}

EventThread::EventThread() {
	ResultCode::AssertOk(trn_thread_create(&thread, EventThread::ThreadEntryShim, this, -1, -2, 0x4000, nullptr));
	interrupt_signal = event_waiter.AddSignal(
		[this]() {
			interrupt_signal->ResetSignal();
			return true;
		});
}

EventThread::~EventThread() {
	StopSync();
	trn_thread_destroy(&thread);
}

void EventThread::Start() {
	thread_running = true;
	exit_requested = false;
	ResultCode::AssertOk(trn_thread_start(&thread));
}

void EventThread::Interrupt() {
	interrupt_signal->Signal();
}

void EventThread::StopAsync() {
	exit_requested = true;
	Interrupt();
}

void EventThread::StopSync() {
	if(thread_running) {
		StopAsync();
		trn_thread_join(&thread, -1);
	}
}

void EventThread::ThreadEntryShim(void *arg) {
	((EventThread*) arg)->ThreadFunc();
}

void EventThread::ThreadFunc() {
	while(!exit_requested) {
		ResultCode::AssertOk(event_waiter.Wait(3000000000));
	}
}

} // namespace util
} // namespace twili
