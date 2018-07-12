#pragma once

#include<unistd.h>

#include<mutex>
#include<memory>

#include "Networking.hpp"
#include "Protocol.hpp"
#include "Buffer.hpp"
#include "Logger.hpp"

namespace twili {
namespace twibc {

template<typename T>
class MessageConnection {
	public:
	template<typename... Args>
	MessageConnection(SOCKET fd, Args&&... args) : obj(std::make_shared<T>(*this, args...)), fd(fd) {
	}
	~MessageConnection() {
		closesocket(fd);
	}

	void PumpOutput() {
		LogMessage(Debug, "pumping out 0x%lx bytes", out_buffer.ReadAvailable());
		std::lock_guard<std::mutex> lock(out_buffer_mutex);
		if(out_buffer.ReadAvailable() > 0) {
			ssize_t r = send(fd, out_buffer.Read(), out_buffer.ReadAvailable(), 0);
			if(r < 0) {
				obj->deletion_flag = true;
				return;
			}
			if(r > 0) {
				out_buffer.MarkRead(r);
			}
		}
	}
	
	void PumpInput() {
		std::tuple<uint8_t*, size_t> target = in_buffer.Reserve(8192);
		ssize_t r = recv(fd, std::get<0>(target), std::get<1>(target), 0);
		if(r <= 0) {
			obj->deletion_flag = true;
			return;
		} else {
			in_buffer.MarkWritten(r);
		}
	}
	
	void Process() {
		while(in_buffer.ReadAvailable() > 0) {
			if(!has_current_mh) {
				if(in_buffer.Read(current_mh)) {
					has_current_mh = true;
				} else {
					in_buffer.Reserve(sizeof(protocol::MessageHeader));
					return;
				}
			}
			
			current_payload.Clear();
			if(in_buffer.Read(current_payload, current_mh.payload_size)) {
				obj->IncomingMessage(current_mh, current_payload);
				has_current_mh = false;
			} else {
				in_buffer.Reserve(current_mh.payload_size);
				return;
			}
		}
	}

	std::shared_ptr<T> obj;
	SOCKET fd;
	util::Buffer in_buffer;

	std::mutex out_buffer_mutex;
	util::Buffer out_buffer;

	bool has_current_mh = false;
	protocol::MessageHeader current_mh;
	util::Buffer current_payload;
};

}
}
