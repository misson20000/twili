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

#pragma once

#include<libtransistor/thread.h>

#include "Threading.hpp"

namespace twili {

class Twili;

class Watchdog {
 public:
	Watchdog(Twili &twili);
 private:
	static void ThreadEntryShim(void *);
	void ThreadFunc();

	std::shared_ptr<trn::WaitHandle> update_handle;
	
	trn_thread_t thread;

	/* these could be atomic, but eeh idc */
	thread::Mutex mutex;
	
	uint64_t last_refresh = 0;
	uint64_t next_deadline = 0;
	uint64_t refresh_period = 1000000000; // 1 second
	uint64_t check_period = 5000000000; // 5 seconds
	uint64_t expiry_period = 5000000000; // 5 seconds

	void Refresh();
};

} // namespace twili
