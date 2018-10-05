#include "MutexShim.hpp"

namespace twili {
namespace util {

MutexShim::MutexShim(trn_mutex_t &mutex) : mutex(mutex) {
}

void MutexShim::lock() {
	trn_mutex_lock(&mutex);
}

bool MutexShim::try_lock() {
	return trn_mutex_try_lock(&mutex);
}

void MutexShim::unlock() {
	trn_mutex_unlock(&mutex);
}

} // namespace util
} // namespace twili
