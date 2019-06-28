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

#include "USBKBackend.hpp"

#include "common/Logger.hpp"
#include "err.hpp"

#include "Daemon.hpp"

namespace twili {
namespace twib {
namespace daemon {
namespace backend {

USBKBackend::USBKBackend(Daemon &daemon) :
	daemon(daemon),
	logic(*this),
	event_loop(logic),
	isl_lock(daemon.initial_scan_lock) {
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

	// unfortunately, libusbK PLUG_ALL_ON_INIT looks like it might be asynchronous,
	// so this doesn't actually do much help...
	if(isl_lock) {
		isl_lock.unlock();
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

	if(device_info->Common.MI == 0) {
		LogMessage(Debug, "  - TransistorUSB serial console identified");
		type = DeviceType::SerialConsole;
	}

	if(device_info->Common.MI == 1) {
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
		LogMessage(Debug, "closing %p", handle);
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

USBKBackend::UsbOvlPool::UsbOvlPool(USBKBackend::UsbHandle &usbd, int count) {
	OvlK_Init(&handle, usbd.handle, count, (KOVL_POOL_FLAG) 0);
}

USBKBackend::UsbOvlPool::~UsbOvlPool() {
	OvlK_Free(handle);
}

USBKBackend::Device::Device(USBKBackend &backend, KUSB_DRIVER_API Usb, UsbHandle &&hnd, uint8_t ep_addrs[4]) :
	backend(backend), Usb(Usb), handle(std::move(hnd)),
	pool(handle, 4),
	member_meta_out(*this, pool, ep_addrs[0], &Device::MetaOutTransferCompleted),
	member_meta_in(*this, pool, ep_addrs[1], &Device::MetaInTransferCompleted),
	member_data_out(*this, pool, ep_addrs[2], &Device::DataOutTransferCompleted),
	member_data_in(*this, pool, ep_addrs[3], &Device::DataInTransferCompleted),
	isl_lock(backend.daemon.initial_scan_lock) {
	LogMessage(Debug, "Device constructor moved %p to %p", hnd.handle, handle.handle);
}

USBKBackend::Device::~Device() {
	for(auto r : pending_requests) {
		if(r.client_id != 0xffffffff) {
			backend.daemon.PostResponse(r.RespondError(TWILI_ERR_PROTOCOL_TRANSFER_ERROR));
		}
	}
}

void USBKBackend::Device::Begin() {
	ResubmitMetaInTransfer();
	
// request identification
	SendRequest(Request(std::shared_ptr<Client>(), 0x0, 0x0, (uint32_t) protocol::ITwibDeviceInterface::Command::IDENTIFY, 0xFFFFFFFF, std::vector<uint8_t>()));
}

void USBKBackend::Device::AddMembers(platform::EventLoop &loop) {
	loop.AddMember(member_data_in);
	loop.AddMember(member_data_out);
	loop.AddMember(member_meta_in);
	loop.AddMember(member_meta_out);
}

void USBKBackend::Device::MarkAdded() {
	if(isl_lock) {
		isl_lock.unlock();
	}
	added_flag = true;
}

void USBKBackend::Device::SendRequest(const Request &&request) {
	std::unique_lock<std::mutex> lock(state_mutex);
	while(state != State::AVAILABLE) {
		state_cv.wait(lock);
	}
	state = State::BUSY;

	mhdr.client_id = request.client ? request.client->client_id : 0xffffffff;
	mhdr.object_id = request.object_id;
	mhdr.command_id = request.command_id;
	mhdr.tag = request.tag;
	mhdr.payload_size = request.payload.size();
	mhdr.object_count = 0;

	request_out = request.Weak();
	pending_requests.push_back(request_out);

	member_meta_out.Submit((uint8_t*)&mhdr, sizeof(mhdr));
	transferring_data = false;
	transferring_meta = true;
}

void USBKBackend::Device::MetaOutTransferCompleted(size_t size) {
	LogMessage(Debug, "finished transferring meta");
	std::unique_lock<std::mutex> lock(state_mutex);

	if(request_out.payload.size() > 0) {
		LogMessage(Debug, "transferring data");
		data_out_transferred = 0;
		member_data_out.Submit((uint8_t*)request_out.payload.data(), LimitTransferSize(request_out.payload.size()));
		transferring_data = true;
	}

	transferring_meta = false;
	if(!transferring_meta && !transferring_data) {
		LogMessage(Debug, "entering AVAILABLE state");
		state = State::AVAILABLE;
		state_cv.notify_one();
	}
}

void USBKBackend::Device::DataOutTransferCompleted(size_t size) {
	data_out_transferred += size;
	size_t remaining = mhdr.payload_size - data_out_transferred;

	if(remaining > 0) {
		member_data_out.Submit((uint8_t*)request_out.payload.data() + data_out_transferred, LimitTransferSize(remaining));
		return;
	} else {
		std::unique_lock<std::mutex> lock(state_mutex);
		
		transferring_data = false;
		if(!transferring_meta && !transferring_data) {
			LogMessage(Debug, "entering AVAILABLE state");
			state = State::AVAILABLE;
			state_cv.notify_one();
		}
	}
}

void USBKBackend::Device::MetaInTransferCompleted(size_t size) {
	response_in.device_id = device_id;
	response_in.client_id = mhdr_in.client_id;
	response_in.object_id = mhdr_in.object_id;
	response_in.result_code = mhdr_in.result_code;
	response_in.tag = mhdr_in.tag;
	response_in.payload.resize(mhdr_in.payload_size);
	object_ids_in.resize(mhdr_in.object_count);
	
	data_in_transferred = 0;
	read_in_objects = 0;
	if(mhdr_in.payload_size > 0) {
		member_data_in.Submit(response_in.payload.data(), LimitTransferSize(response_in.payload.size()));
	} else if(mhdr_in.object_count > 0) {
		member_data_in.Submit((uint8_t*) object_ids_in.data(), object_ids_in.size() * sizeof(uint32_t));
	} else {
		DispatchResponse();
	}
}

void USBKBackend::Device::DataInTransferCompleted(size_t size) {
	if(data_in_transferred < mhdr_in.payload_size) {
		data_in_transferred += size;
		size_t remaining = mhdr_in.payload_size - data_in_transferred;

		if(remaining > 0) {
			// continue transferring
			member_data_in.Submit(response_in.payload.data() + data_in_transferred, LimitTransferSize(remaining));
			return;
		}

		if(mhdr_in.object_count > 0) {
			member_data_in.Submit((uint8_t*)object_ids_in.data(), object_ids_in.size() * sizeof(uint32_t));
			return;
		} else {
			DispatchResponse();
		}
	} else {
		if(size != object_ids_in.size() * sizeof(uint32_t)) {
			LogMessage(Error, "bad object ids in size");
			deletion_flag = true;
			return;
		}
		DispatchResponse();
	}
}

void USBKBackend::Device::DispatchResponse() {
	// create BridgeObjects
	response_in.objects.resize(object_ids_in.size());
	std::transform(
		object_ids_in.begin(), object_ids_in.end(), response_in.objects.begin(),
		[this](uint32_t id) {
			return std::make_shared<BridgeObject>(backend.daemon, response_in.device_id, id);
		});

	// remove from pending requests
	pending_requests.remove_if([this](WeakRequest &r) {
			return r.tag == response_in.tag;
		});
	
	if(response_in.client_id == 0xFFFFFFFF) { // identification meta-client
		Identified(response_in);
	} else {
		backend.daemon.PostResponse(std::move(response_in));
	}
	ResubmitMetaInTransfer();
}

void USBKBackend::Device::Identified(Response &r) {
	LogMessage(Debug, "got identification response back");
	LogMessage(Debug, "payload size: 0x%x", r.payload.size());
	if(r.result_code != 0) {
		LogMessage(Warning, "device identification error: 0x%x", r.result_code);
		deletion_flag = true;
		return;
	}
	std::string err;
	msgpack11::MsgPack obj = msgpack11::MsgPack::parse(std::string(r.payload.begin() + 8, r.payload.end()), err);
	identification = obj;
	device_nickname = obj["device_nickname"].string_value();
	serial_number = obj["serial_number"].string_value();

	LogMessage(Info, "nickname: %s", device_nickname.c_str());
	LogMessage(Info, "serial number: %s", serial_number.c_str());
	
	device_id = std::hash<std::string>()(serial_number);
	LogMessage(Info, "assigned device id: %08x", device_id);
	ready_flag = true;
}

void USBKBackend::Device::ResubmitMetaInTransfer() {
	LogMessage(Debug, "submitting meta in transfer");
	member_meta_in.Submit((uint8_t*)&mhdr_in, sizeof(mhdr_in));
}

size_t USBKBackend::Device::LimitTransferSize(size_t sz) {
	const size_t max_size = 0x10000;
	if(sz > max_size) {
		return max_size;
	} else {
		return sz;
	}
}

int USBKBackend::Device::GetPriority() {
	return 3; // higher priority than TCP and LibUSB
}

std::string USBKBackend::Device::GetBridgeType() {
	return "usbk";
}

USBKBackend::Device::TransferMember::TransferMember(Device &device, UsbOvlPool &pool, uint8_t ep_addr, void (Device::*callback)(size_t)) :
	device(device),
	ep_addr(ep_addr),
	callback(callback),
	event(INVALID_HANDLE_VALUE) {
	OvlK_Acquire(&overlap, pool.handle);
	event = OvlK_GetEventHandle(overlap);
}

USBKBackend::Device::TransferMember::~TransferMember() {
	event.Claim();
	OvlK_Release(overlap);
}

void USBKBackend::Device::TransferMember::Submit(uint8_t *buffer, size_t size) {
	if(is_active) {
		LogMessage(Error, "tried to submit transfer while already transferring");
		return;
	}
	OvlK_ReUse(overlap);
	bool ret;
	if(USB_ENDPOINT_DIRECTION_IN(ep_addr)) {
		ret = device.Usb.ReadPipe(device.handle.handle, ep_addr, buffer, size, nullptr, (LPOVERLAPPED)overlap);
	} else {
		ret = device.Usb.WritePipe(device.handle.handle, ep_addr, buffer, size, nullptr, (LPOVERLAPPED)overlap);
	}
	if(ret) {
		LogMessage(Debug, "succeeded instantly?");
	} else {
		int err = GetLastError();
		if(err != ERROR_IO_PENDING) {
			LogMessage(Debug, "submit failed: %d", GetLastError());
			device.deletion_flag = true;
		} else {
			is_active = true;
		}
	}
}

bool USBKBackend::Device::TransferMember::WantsSignal() {
	return true;
}

void USBKBackend::Device::TransferMember::Signal() {
	UINT actual_length;
	if(OvlK_Wait(overlap, 0, KOVL_WAIT_FLAG_NONE, &actual_length)) {
		is_active = false;
		ResetEvent(event.handle);
		std::invoke(callback, device, (size_t) actual_length);
	} else {
		LogMessage(Debug, "a transfer member signalled failure");
		device.deletion_flag = true;
	}
}

platform::windows::Event &USBKBackend::Device::TransferMember::GetEvent() {
	return event;
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
	bool success = true;
	success &= Usb.QueryPipe(handle.handle, 0, 0, &pipe_meta_out_info);
	success &= Usb.QueryPipe(handle.handle, 0, 1, &pipe_data_out_info);
	success &= Usb.QueryPipe(handle.handle, 0, 2, &pipe_meta_in_info);
	success &= Usb.QueryPipe(handle.handle, 0, 3, &pipe_data_in_info);
	if(!success) {
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

	uint8_t addrs[] = {
		pipe_meta_out_info.PipeId,
		pipe_meta_in_info.PipeId,
		pipe_data_out_info.PipeId,
		pipe_data_in_info.PipeId
	};

	devices.emplace_back(std::make_shared<Device>(*this, Usb, std::move(handle), addrs))->Begin();
	event_loop.GetNotifier().Notify();
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
	event_loop.GetNotifier().Notify();
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

void USBKBackend::Logic::Prepare(platform::EventLoop &loop) {
	loop.Clear();
	for(auto i = backend.stdout_transfers.begin(); i != backend.stdout_transfers.end();) {
		if((*i)->deletion_flag) {
			i = backend.stdout_transfers.erase(i);
			continue;
		}
		loop.AddMember(**i);
		i++;
	}
	for(auto i = backend.devices.begin(); i != backend.devices.end(); ) {
		auto d = *i;
		if(d->deletion_flag) {
			if(d->added_flag) {
				backend.daemon.RemoveDevice(d);
			}
			i = backend.devices.erase(i);
			continue;
		}

		if(d->ready_flag && !d->added_flag) {
			backend.daemon.AddDevice(d);
			d->MarkAdded();
		}

		d->AddMembers(loop);
		i++;
	}
}

} // namespace backend
} // namespace daemon
} // namespace twib
} // namespace twili
