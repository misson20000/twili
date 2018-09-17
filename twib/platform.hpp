#pragma once

// cross-platform networking includes and defines

// this needs to be included super early, because
// windows.h is a load of garbage and includes
// winsock.h, which is incompatible with winsock2.h,
// if we don't include winsock2.h first.

#ifdef _WIN32
#ifdef _WINDOWS_
#error windows already included
#endif
#define NOMINMAX
#include<Winsock2.h>
#include<WS2tcpip.h>
#include<io.h>
typedef signed long long ssize_t;

static inline char *NetErrStr() {
	char *s = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&s, 0, NULL);
	return s;
}

#else

#include<sys/socket.h>
#include<sys/select.h>
#include<sys/un.h>
#include<netinet/in.h>
#include<unistd.h>
#include<errno.h>

typedef int SOCKET;
#define INVALID_SOCKET -1
#define closesocket close

static inline char *NetErrStr() {
	return strerror(errno);
}

#endif
