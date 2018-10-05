#pragma once

namespace twili {
namespace util {

class Socket {
 public:
	Socket() = default;
	Socket(int fd);
	~Socket();

	Socket(Socket &&other);
	Socket &operator=(Socket &&other);

	Socket(const Socket&) = delete;
	Socket &operator=(const Socket&) = delete;

	void Close();
	
	int fd = -1;
};

} // namespace util
} // namespace twili
