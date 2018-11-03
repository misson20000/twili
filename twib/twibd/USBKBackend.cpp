#include "USBKBackend.hpp"

#include "Logger.hpp"

namespace twili {
namespace twibd {
namespace backend {

USBKBackend::USBKBackend(Twibd &twibd) : twibd(twibd), logic(*this), event_loop(logic) {
	if(!LibK_Context_Init(nullptr, nullptr)) {
		LogMessage(Fatal, "Failed to initialize usbK context: %d", GetLastError());
		exit(1);
	}
	LogMessage(Debug, "constructed USBKBackend");
	event_loop.Begin();
}

USBKBackend::~USBKBackend() {
	event_loop.Destroy();
	if(hot_handle) {
		HotK_Free(hot_handle);
	}
	LibK_Context_Free();
}

void USBKBackend::Probe() {
	LogMessage(Debug, "probing");
	KHOT_PARAMS hot_params = { 0 };
	hot_params.OnHotPlug = &USBKBackend::hotplug_notify_shim;
	hot_params.Flags = KHOT_FLAG_PLUG_ALL_ON_INIT;
	strncpy(hot_params.PatternMatch.DeviceInterfaceGUID, "*", KLST_STRING_MAX_LEN); // just pattern match everything :shrug:

	// I really hate this.
	LibK_SetDefaultContext(KLIB_HANDLE_TYPE_HOTK, (KLIB_USER_CONTEXT)this);
	if(!HotK_Init(&hot_handle, &hot_params)) {
		LogMessage(Fatal, "Hotplug initialization failed: %d", GetLastError());
		exit(1);
	}
}

void USBKBackend::DetectDevice(KLST_DEVINFO_HANDLE device_info) {
	LogMessage(Debug, "detected device:");
	LogMessage(Debug, "  desc: %s", device_info->DeviceDesc);
	LogMessage(Debug, "  mfg: %s", device_info->Mfg);
	LogMessage(Debug, "  service: %s", device_info->Service);
	LogMessage(Debug, "  deviceID: %s", device_info->DeviceID);
	LogMessage(Debug, "  deviceInterfaceGUID: %s", device_info->DeviceInterfaceGUID);
	LogMessage(Debug, "  devicePath: %s", device_info->DevicePath);
	if(device_info->Common.Vid == TWILI_VENDOR_ID &&
		device_info->Common.Pid == TWILI_PRODUCT_ID || (
			TWIBD_ACCEPT_NINTENDO_SDK_DEBUGGER &&
			device_info->Common.Vid == TWIBD_NINTENDO_SDK_DEBUGGER_PRODUCT_ID &&
			device_info->Common.Pid == TWIBD_NINTENDO_SDK_DEBUGGER_VENDOR_ID)) {
		AddDevice(device_info);
	}
}

enum class DeviceType {
	Unknown, SerialConsole, Twili
};

void USBKBackend::AddDevice(KLST_DEVINFO_HANDLE device_info) {
	LogMessage(Debug, "found device (%s - %s) MI %d", device_info->DeviceDesc,
		device_info->Mfg, device_info->Common.MI);

	DeviceType type = DeviceType::Unknown;

	if(!strcmp(device_info->DeviceInterfaceGUID, "{97BF784F-6433-3877-36EA-76C5314F7F46}")) {
		LogMessage(Debug, "  - TransistorUSB serial console identified");
		type = DeviceType::SerialConsole;
	}

	if(!strcmp(device_info->DeviceInterfaceGUID, "{67437937-72E1-5CAF-99CA-71FD46545AE2}")) {
		LogMessage(Debug, "  - TransistorUSB twili device identified");
		type = DeviceType::Twili;
	}

	if(type == DeviceType::Unknown) {
		return;
	}

	if(type == DeviceType::Twili) {
		AddTwiliDevice(device_info);
	} else if(type == DeviceType::SerialConsole) {
		AddSerialConsole(device_info);
	}
	return;
}

void USBKBackend::hotplug_notify_shim(KHOT_HANDLE hot_handle, KLST_DEVINFO_HANDLE device_info, KLST_SYNC_FLAG plug_type) {
	if(plug_type == KLST_SYNC_FLAG_ADDED) {
		((USBKBackend*)LibK_GetContext(hot_handle, KLIB_HANDLE_TYPE_HOTK))->DetectDevice(device_info);
	} else if(plug_type == KLST_SYNC_FLAG_REMOVED) {
		LogMessage(Debug, "a device was removed, but not sure which one");
	}
}

USBKBackend::UsbHandle::UsbHandle() {

}

USBKBackend::UsbHandle::UsbHandle(KUSB_HANDLE handle) : handle(handle) {

}

USBKBackend::UsbHandle::UsbHandle(UsbHandle &&other) : handle(other.handle) {
	other.handle = nullptr;
}

USBKBackend::UsbHandle::~UsbHandle() {
	if(handle != nullptr) {
		UsbK_Free(handle);
	}
}

USBKBackend::UsbHandle &USBKBackend::UsbHandle::operator=(UsbHandle &&other) {
	if(handle != nullptr) {
		UsbK_Free(handle);
	}
	handle = other.handle;
	other.handle = nullptr;
	return *this;
}

USBKBackend::Device::Device(USBKBackend &backend, KUSB_DRIVER_API Usb,  UsbHandle &&handle) : backend(backend), Usb(Usb), handle(std::move(handle)) {

}

USBKBackend::Device::~Device() {

}

void USBKBackend::Device::Begin() {

}

void USBKBackend::Device::SendRequest(const Request &&r) {

}

int USBKBackend::Device::GetPriority() {
	return 3; // higher priority than TCP and LibUSB
}

std::string USBKBackend::Device::GetBridgeType() {
	return "usbk";
}

void USBKBackend::AddTwiliDevice(KLST_DEVINFO_HANDLE device_info) {
	UsbHandle handle;
	KUSB_DRIVER_API Usb;
	if(!LibK_LoadDriverAPI(&Usb, device_info->DriverID)) {
		LogMessage(Error, "failed to load driver API: %d", GetLastError());
		return;
	}

	if(!Usb.Init(&handle.handle, device_info)) {
		LogMessage(Error, "failed to open USB device: %d", GetLastError());
		return;
	}

	WINUSB_PIPE_INFORMATION pipe_meta_out_info;
	WINUSB_PIPE_INFORMATION pipe_data_out_info;
	WINUSB_PIPE_INFORMATION pipe_meta_in_info;
	WINUSB_PIPE_INFORMATION pipe_data_in_info;
	bool fail = false;
	fail |= Usb.QueryPipe(handle.handle, 0, 0, &pipe_meta_out_info);
	fail |= Usb.QueryPipe(handle.handle, 0, 1, &pipe_data_out_info);
	fail |= Usb.QueryPipe(handle.handle, 0, 2, &pipe_meta_in_info);
	fail |= Usb.QueryPipe(handle.handle, 0, 3, &pipe_data_in_info);
	if(fail) {
		LogMessage(Error, "failed to query pipe: %d", GetLastError());
		return;
	}

	if(!USB_ENDPOINT_DIRECTION_OUT(pipe_meta_out_info.PipeId) ||
		!USB_ENDPOINT_DIRECTION_OUT(pipe_data_out_info.PipeId) ||
		!USB_ENDPOINT_DIRECTION_IN(pipe_meta_in_info.PipeId) ||
		!USB_ENDPOINT_DIRECTION_IN(pipe_data_in_info.PipeId)) {
		LogMessage(Warning, "Twili interface exposes endpoints with bad directions");
		return;
	}

	if(pipe_meta_out_info.PipeType != USB_ENDPOINT_TYPE_BULK ||
		pipe_data_out_info.PipeType != USB_ENDPOINT_TYPE_BULK ||
		pipe_meta_in_info.PipeType != USB_ENDPOINT_TYPE_BULK ||
		pipe_data_in_info.PipeType != USB_ENDPOINT_TYPE_BULK) {
		LogMessage(Warning, "Twili interface exposes endpoints with bad transfer types");
		return;
	}

	devices.emplace_back(std::make_shared<Device>(*this, Usb, std::move(handle)))->Begin();
}

void USBKBackend::AddSerialConsole(KLST_DEVINFO_HANDLE device_info) {
	UsbHandle handle;
	KUSB_DRIVER_API Usb;
	if(!LibK_LoadDriverAPI(&Usb, device_info->DriverID)) {
		LogMessage(Error, "failed to load driver API: %d", GetLastError());
		return;
	}

	if(!Usb.Init(&handle.handle, device_info)) {
		LogMessage(Error, "failed to open USB device: %d", GetLastError());
		return;
	}

	WINUSB_PIPE_INFORMATION pipe_out_info;
	WINUSB_PIPE_INFORMATION pipe_in_info;
	if(!Usb.QueryPipe(handle.handle, 0, 0, &pipe_out_info)) {
		LogMessage(Error, "failed to query pipe: %d", GetLastError());
		return;
	}
	
	if(!Usb.QueryPipe(handle.handle, 0, 1, &pipe_in_info)) {
		LogMessage(Error, "failed to query pipe: %d", GetLastError());
		return;
	}

	if(!USB_ENDPOINT_DIRECTION_OUT(pipe_out_info.PipeId) ||
		!USB_ENDPOINT_DIRECTION_IN(pipe_in_info.PipeId)) {
		LogMessage(Warning, "stdio interface exposes endpoints with bad directions");
		return;
	}

	if(pipe_out_info.PipeType != USB_ENDPOINT_TYPE_BULK ||
		pipe_in_info.PipeType != USB_ENDPOINT_TYPE_BULK) {
		LogMessage(Warning, "stdio interface exposes endpoints with bad transfer types");
		return;
	}

	stdout_transfers.emplace_back(std::make_shared<StdoutTransferState>(*this, Usb, std::move(handle), pipe_in_info.PipeId))->Submit();
	event_loop.notifier.Notify();
}

USBKBackend::StdoutTransferState::StdoutTransferState(USBKBackend &backend, KUSB_DRIVER_API Usb, UsbHandle &&hnd, uint8_t ep_addr) :
	backend(backend),
	Usb(Usb),
	handle(std::move(hnd)),
	event(INVALID_HANDLE_VALUE),
	ep_addr(ep_addr) {
	OvlK_Init(&pool, handle.handle, 1, (KOVL_POOL_FLAG) 0);
	OvlK_Acquire(&overlap, pool);
	event = OvlK_GetEventHandle(overlap);
}

USBKBackend::StdoutTransferState::~StdoutTransferState() {
	event.Claim();
	OvlK_Release(overlap);
	OvlK_Free(pool);
}

void USBKBackend::StdoutTransferState::Submit() {
	OvlK_ReUse(overlap);
	if(Usb.ReadPipe(handle.handle, ep_addr, io_buffer, sizeof(io_buffer), nullptr, (LPOVERLAPPED)overlap)) {
		LogMessage(Debug, "succeeded instantly?");
	} else {
		int err = GetLastError();
		if(err != ERROR_IO_PENDING) {
			LogMessage(Debug, "submit failed: %d", GetLastError());
			Kill();
		}
	}
}

void USBKBackend::StdoutTransferState::Kill() {
	deletion_flag = true;
}

bool USBKBackend::StdoutTransferState::WantsSignal() {
	return !deletion_flag;
}

void USBKBackend::StdoutTransferState::Signal() {
	UINT actual_length;
	if(OvlK_Wait(overlap, 0, KOVL_WAIT_FLAG_NONE, &actual_length)) {
		string_buffer.Write(io_buffer, actual_length);
		for(bool found_line = true; found_line; found_line = false) {
			char *current_line = (char*) string_buffer.Read();
			for(size_t i = 0; i < string_buffer.ReadAvailable(); i++) {
				if(current_line[i] == '\n') {
					LogMessage(Message, "[TWILI] %.*s", i, current_line);
					string_buffer.MarkRead(i+1);
					found_line = true;
					break;
				}
			}
		}
		Submit();
	} else {
		int err = GetLastError();
		LogMessage(Debug, "stdio transfer failed: %d", err);
		Kill();
	}
}

platform::windows::Event &USBKBackend::StdoutTransferState::GetEvent() {
	return event;
}

USBKBackend::Logic::Logic(USBKBackend &backend) : backend(backend) {

}

void USBKBackend::Logic::Prepare(platform::windows::EventLoop &loop) {
	loop.Clear();
	for(std::shared_ptr<StdoutTransferState> &ptr : backend.stdout_transfers) {
		loop.AddMember(*ptr);
	}
}

} // namespace backend
} // namespace twibd
} // namespace twili
