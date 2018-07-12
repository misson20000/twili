#pragma once

// cross-platform networking includes and defines

#ifdef _WIN32

#include<winsock2.h>

#else

#include<sys/socket.h>
#include<sys/select.h>

#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket close

#endif
