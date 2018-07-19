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
#else

#include<sys/socket.h>
#include<sys/select.h>
#include<sys/un.h>
#include<netinet/in.h>
#include<unistd.h>

typedef int SOCKET;
#define INVALID_SOCKET -1
#define closesocket close

#endif
