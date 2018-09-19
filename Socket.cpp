#include "Socket.hpp"

#ifdef __SWITCH__
#include<libtransistor/ipc/bsd.h>
#define closesocket bsd_close
#else
#include "platform.hpp"
#endif

namespace twili {
namespace util {

Socket::Socket(int fd) : fd(fd) {
}

Socket::~Socket() {
	if(fd != -1) {
		closesocket(fd);
	}
}

Socket::Socket(Socket &&other) : fd(other.fd) {
	other.fd = -1;
}

Socket &Socket::operator=(Socket &&other) {
	if(fd != -1) {
		closesocket(fd);
	}
	fd = other.fd;
	other.fd = -1;
	
	return *this;
}

void Socket::Close() {
	if(fd != -1) {
		closesocket(fd);
		fd = -1;
	}
}

} // namespace util
} // namespace twili
