//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2020 misson20000 <xenotoad@xenotoad.net>
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

#include "Watchdog.hpp"

#include<libtransistor/cpp/nx.hpp>

#include<mutex>

#include "twili.hpp"

namespace twili {

static uint64_t ticks_to_ns(uint64_t ticks) {
	return ticks * 10000 / 192;
}

static uint64_t ns_to_ticks(uint64_t ns) {
	return ns * 192 / 10000;
}

Watchdog::Watchdog(Twili &twili) {
	this->next_deadline = svcGetSystemTick();
	this->Refresh();
	
	this->update_handle = twili.event_waiter.AddDeadline(
		this->next_deadline,
		[this]() -> uint64_t {
			std::unique_lock<thread::Mutex> lock(this->mutex);
			this->Refresh();
			return this->next_deadline;
		});

	twili::Assert(trn_thread_create(&this->thread, ThreadEntryShim, this, 30, -2, 0x1000, nullptr));
	twili::Assert(trn_thread_start(&this->thread));
}

void Watchdog::Refresh() {
	this->last_refresh = svcGetSystemTick();
	this->next_deadline+= ns_to_ticks(this->refresh_period);
}

void Watchdog::ThreadEntryShim(void *arg) {
	((Watchdog *) arg)->ThreadFunc();
}

void Watchdog::ThreadFunc() {
	while(true) {
		svcSleepThread(this->check_period);

		{
			std::unique_lock<thread::Mutex> lock(this->mutex);
			if(ticks_to_ns(svcGetSystemTick() - this->last_refresh) > this->expiry_period) {
				printf("watchdog expired\n");
				twili::Abort(TWILI_ERR_WATCHDOG_EXPIRED);
			}
		}
	}
}

} // namespace twili
