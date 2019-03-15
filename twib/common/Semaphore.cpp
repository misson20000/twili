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

#include "Semaphore.hpp"

#include<mutex>

namespace twili {
namespace twib {
namespace common {

Semaphore::Semaphore(size_t count) : count(count) {

}

void Semaphore::notify() {
	std::lock_guard<std::mutex> lock(mutex);
	++count;
	condition_variable.notify_one();
}

void Semaphore::wait() {
	std::unique_lock<std::mutex> lock(mutex);
	condition_variable.wait(lock, [&] { return count > 0; });
	--count;
}

void Semaphore::lock() {
	wait();
}

void Semaphore::unlock() {
	notify();
}

} // namespace common
} // namespace twib
} // namespace twili