#pragma once

#include "platform.hpp"

#include<list>
#include<thread>
#include<mutex>
#include<variant>
#include<map>
#include<random>

#include "blockingconcurrentqueue.h"

#include "config.hpp"
#if TWIBD_TCP_BACKEND_ENABLED
#include "TCPBackend.hpp"
#endif
#if TWIBD_LIBUSB_BACKEND_ENABLED
#include "USBBackend.hpp"
#endif
#if TWIBD_LIBUSBK_BACKEND_ENABLED
#include "USBKBackend.hpp"
#endif

#include "Logger.hpp"
#include "Messages.hpp"
#include "Device.hpp"
#include "LocalClient.hpp"

namespace twili {
namespace twibd {

class Twibd {
 public:
	Twibd();
	~Twibd();

	void AddDevice(std::shared_ptr<Device> device);
	void AddClient(std::shared_ptr<Client> client);
	void PostRequest(Request &&request);
	void PostResponse(Response &&response);
	void RemoveDevice(std::shared_ptr<Device> device);
	void RemoveClient(std::shared_ptr<Client> client);
	
	void Process();
	Response HandleRequest(Request &request);
	std::shared_ptr<Client> GetClient(uint32_t client_id);

	std::shared_ptr<LocalClient> local_client;
 private:
#if TWIBD_TCP_BACKEND_ENABLED
	backend::TCPBackend tcp;
#endif
#if TWIBD_LIBUSB_BACKEND_ENABLED
	backend::USBBackend usb;
#endif
#if TWIBD_LIBUSBK_BACKEND_ENABLED
	backend::USBKBackend usbk;
#endif

	moodycamel::BlockingConcurrentQueue<std::variant<Request, Response>> dispatch_queue;
	
	std::mutex device_map_mutex;
	std::map<uint32_t, std::weak_ptr<Device>> devices;
	
	std::mutex client_map_mutex;
	std::map<uint32_t, std::weak_ptr<Client>> clients;

	std::random_device rng;
};

} // namespace twibd
} // namespace twili
