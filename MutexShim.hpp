#pragma once

#include<libtransistor/mutex.h>

namespace twili {
namespace util {

class MutexShim {
 public:
	MutexShim(trn_mutex_t &mutex);
	void lock();
	bool try_lock();
	void unlock();
 private:
	trn_mutex_t &mutex;
};

} // namespace util
} // namespace twili
