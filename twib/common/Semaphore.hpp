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

#include<mutex>
#include<condition_variable>

#include<stdint.h>

namespace twili {
namespace twib {
namespace common {

class Semaphore {
public:
	Semaphore(size_t count = 0);
	void notify();
	void wait();

	// for std::lock_guard
	void lock();
	void unlock();
private:
	std::mutex mutex;
	std::condition_variable condition_variable;
	size_t count = 0;
};

} // namespace common
} // namespace twib
} // namespace twili