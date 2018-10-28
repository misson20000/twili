#pragma once

#include "platform.hpp"

#include<thread>
#include<list>
#include<queue>
#include<mutex>
#include<condition_variable>

#include<libusbk.h>

#include "Buffer.hpp"
#include "Device.hpp"
#include "Messages.hpp"
#include "Protocol.hpp"

namespace twili {
namespace twibd {

class Twibd;

namespace backend {

class USBKBackend {
 public:
	USBKBackend(Twibd &twibd);
	~USBKBackend();
	
	void Probe();

	class Device : public twibd::Device, public std::enable_shared_from_this<Device> {
	 public:
		Device(USBKBackend &backend, KUSB_HANDLE handle, uint8_t endp_addrs[4], uint8_t interface_number);
		~Device();

		enum class State {
			AVAILABLE, BUSY
		};

		void Begin();
		
		// thread-agnostic
		virtual void SendRequest(const Request &&r) override;

		virtual int GetPriority() override;
		virtual std::string GetBridgeType() override;
		
		bool ready_flag = false;
		bool added_flag = false;
	 private:
		USBKBackend &backend;
		
		KUSB_HANDLE handle;
	};

	void AddDevice(KLST_DEVINFO_HANDLE device_info);

 private:
	Twibd &twibd;
	std::list<std::shared_ptr<Device>> devices;
	
	KHOT_HANDLE hot_handle = nullptr;

	bool event_thread_destroy = false;
	void event_thread_func();
	std::thread event_thread;

	static void hotplug_notify_shim(KHOT_HANDLE hot_handle, KLST_DEVINFO_HANDLE device_info, KLST_SYNC_FLAG plug_type);
};

} // namespace backend
} // namespace twibd
} // namespace twili
