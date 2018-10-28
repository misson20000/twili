#include "USBKBackend.hpp"

#include "Logger.hpp"

namespace twili {
namespace twibd {
namespace backend {

USBKBackend::USBKBackend(Twibd &twibd) : twibd(twibd) {
	if(!LibK_Context_Init(nullptr, nullptr)) {
		LogMessage(Fatal, "Failed to initialize usbK context: %d", GetLastError());
		exit(1);
	}
	LogMessage(Debug, "constructed USBKBackend");
}

USBKBackend::~USBKBackend() {
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
	LibK_SetDefaultContext(KLIB_HANDLE_TYPE_HOTK, (KLIB_USER_CONTEXT) this);
	if(!HotK_Init(&hot_handle, &hot_params)) {
		LogMessage(Fatal, "Hotplug initialization failed: %d", GetLastError());
		exit(1);
	}
}

void USBKBackend::AddDevice(KLST_DEVINFO_HANDLE device_info) {
	LogMessage(Debug, "detected device:");
	LogMessage(Debug, "  desc: %s", device_info->DeviceDesc);
	LogMessage(Debug, "  mfg: %s", device_info->Mfg);
	LogMessage(Debug, "  service: %s", device_info->Service);
	LogMessage(Debug, "  deviceID: %s", device_info->DeviceID);
	LogMessage(Debug, "  deviceInterfaceGUID: %s", device_info->DeviceInterfaceGUID);
	LogMessage(Debug, "  devicePath: %s", device_info->DevicePath);
}

void USBKBackend::hotplug_notify_shim(KHOT_HANDLE hot_handle, KLST_DEVINFO_HANDLE device_info, KLST_SYNC_FLAG plug_type) {
	if(plug_type == KLST_SYNC_FLAG_ADDED) {
		((USBKBackend*) LibK_GetContext(hot_handle, KLIB_HANDLE_TYPE_HOTK))->AddDevice(device_info);
	} else if(plug_type == KLST_SYNC_FLAG_REMOVED) {
		LogMessage(Debug, "a device was removed, but not sure which one");
	}
}


} // namespace backend
} // namespace twibd
} // namespace twili
