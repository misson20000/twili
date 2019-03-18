//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2018 misson20000 <xenotoad@xenotoad.net>
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

#pragma once

#include "platform/platform.hpp"

#include<list>
#include<thread>
#include<mutex>
#include<variant>
#include<map>
#include<random>

#include "common/blockingconcurrentqueue.h"
#include "common/config.hpp"
#include "common/Logger.hpp"

#if TWIBD_TCP_BACKEND_ENABLED
#include "TCPBackend.hpp"
#endif
#if TWIBD_LIBUSB_BACKEND_ENABLED
#include "USBBackend.hpp"
#endif
#if TWIBD_LIBUSBK_BACKEND_ENABLED
#include "USBKBackend.hpp"
#endif

#include "Messages.hpp"
#include "Device.hpp"
#include "LocalClient.hpp"

namespace twili {
namespace twib {
namespace daemon {

class Daemon {
 public:
	Daemon();
	~Daemon();

	void AddDevice(std::shared_ptr<Device> device);
	void AddClient(std::shared_ptr<Client> client);
	void Awaken();
	void PostRequest(Request &&request);
	void PostResponse(Response &&response);
	void RemoveDevice(std::shared_ptr<Device> device);
	void RemoveClient(std::shared_ptr<Client> client);
	
	void Process();
	Response HandleRequest(Request &request);
	std::shared_ptr<Client> GetClient(uint32_t client_id);

	std::shared_ptr<LocalClient> local_client;
 private:
	moodycamel::BlockingConcurrentQueue<std::variant<std::monostate, Request, Response>> dispatch_queue;
	
	std::mutex device_map_mutex;
	std::map<uint32_t, std::weak_ptr<Device>> devices;
	
	std::mutex client_map_mutex;
	std::map<uint32_t, std::weak_ptr<Client>> clients;

	std::random_device rng;

#if TWIBD_TCP_BACKEND_ENABLED
	backend::TCPBackend tcp;
#endif
#if TWIBD_LIBUSB_BACKEND_ENABLED
	backend::USBBackend usb;
#endif
#if TWIBD_LIBUSBK_BACKEND_ENABLED
	backend::USBKBackend usbk;
#endif
};

} // namespace daemon
} // namespace twib
} // namespace twili
