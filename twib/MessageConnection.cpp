#include "MessageConnection.hpp"

namespace twili {
namespace twibc {

MessageConnection::MessageConnection(T &&obj, SOCKET fd) : obj(std::make_shared<T>(std::move(obj))), fd(fd) {
	
}

MessageConnection::~MessageConnection() {
	closesocket(fd);
}

void MessageConnection::PumpOutput() {
}

void MessageConnection::PumpInput() {
}

void MessageConnection::Process() {
}

} // namespace twibc
} // namespace twili
