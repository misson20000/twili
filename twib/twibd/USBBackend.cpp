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

#include "USBBackend.hpp"

#include<msgpack11.hpp>

#include "Twibd.hpp"
#include "config.hpp"
#include "err.hpp"

//#ifdef _WIN32
//#include<libusbk.h>
//#endif

void show(msgpack11::MsgPack const& blob);

namespace twili {
namespace twibd {
namespace backend {

//#ifdef _WIN32
//void KUSB_API hotplug_cb_shim(KHOT_HANDLE handle, KLST_DEVINFO_HANDLE DeviceInfo, KLST_SYNC_FLAG NotificationType) {
//	libusb_device **list;
//	if (NotificationType == KLST_SYNC_FLAG_ADDED) {
//		// Open device with libusb now.
//		auto cnt = libusb_get_device_list(NULL, &list);
//		if (cnt < 0) {
//			LogMessage(Error, "Failed to get device list: %d", cnt);
//			exit(1);
//		}
//		for (auto i = 0; i < cnt; i++) {
//			libusb_device *device = list[i];
//			libusb_device_descriptor desc;
//			if (libusb_get_device_descriptor(device, &desc) < 0) {
//				LogMessage(Error, "Failed to get device descriptor");
//				exit(1);
//			}
//			printf("%d:%d vs %d:%d (This is %d)\n", DeviceInfo->BusNumber, DeviceInfo->DeviceAddress, libusb_get_bus_number(device), libusb_get_port_number(device), desc.idVendor, desc.idProduct);
//		}
//		exit(1);
//		//self->QueueAddDevice(device->);
//	} else {
//		//self->RemoveDevice(/* get ctx and device */);
//	}
//}
//#else
int hotplug_cb_shim(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *user_data) {
	USBBackend *self = (USBBackend*) user_data;
	if(event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
		self->QueueAddDevice(device);
	} else if(event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
		self->RemoveDevice(ctx, device);
	}
	return 0;
}
//#endif

USBBackend::USBBackend(Twibd *twibd) : twibd(twibd) {
	int r = libusb_init(&ctx);
	if(r) {
		LogMessage(Fatal, "failed to initialize libusb: %s", libusb_error_name(r));
		exit(1);
	}

	std::thread event_thread(&USBBackend::event_thread_func, this);
	this->event_thread = std::move(event_thread);
}

USBBackend::~USBBackend() {
	event_thread_destroy = true;
	if(TWIBD_HOTPLUG_ENABLED && libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
		libusb_hotplug_deregister_callback(ctx, hotplug_handle); // wakes up usb_event_thread
		if(TWIBD_ACCEPT_NINTENDO_SDK_DEBUGGER) {
			libusb_hotplug_deregister_callback(ctx, hotplug_handle_nintendo_sdk_debugger);
		}
	}
	event_thread.join();
	libusb_exit(ctx);
}

USBBackend::Device::Device(USBBackend *backend, libusb_device_handle *handle, uint8_t endp_addrs[4], uint8_t interface_number) :
	backend(backend), handle(handle),
	endp_meta_out(endp_addrs[0]), endp_meta_in(endp_addrs[2]),
	endp_data_out(endp_addrs[1]), endp_data_in(endp_addrs[3]),
	interface_number(interface_number) {
	
	tfer_meta_out = libusb_alloc_transfer(0);
	tfer_data_out = libusb_alloc_transfer(0);
	tfer_meta_in = libusb_alloc_transfer(0);
	tfer_data_in = libusb_alloc_transfer(0);
}

USBBackend::Device::~Device() {
	for(auto r : pending_requests) {
		if(r.client_id != 0xffffffff) {
			backend->twibd->PostResponse(r.RespondError(TWILI_ERR_PROTOCOL_TRANSFER_ERROR));
		}
	}
	libusb_free_transfer(tfer_meta_out);
	libusb_free_transfer(tfer_data_out);
	libusb_free_transfer(tfer_meta_in);
	libusb_free_transfer(tfer_data_in);
	libusb_release_interface(handle, interface_number);
	libusb_close(handle);
}

void USBBackend::Device::Begin() {
	ResubmitMetaInTransfer();

	// request identification
	SendRequest(Request(std::shared_ptr<Client>(), 0x0, 0x0, (uint32_t) protocol::ITwibDeviceInterface::Command::IDENTIFY, 0xFFFFFFFF, std::vector<uint8_t>()));
}

void USBBackend::Device::SendRequest(const Request &&request) {
	std::unique_lock<std::mutex> lock(state_mutex);
	while(state != State::AVAILABLE) {
		state_cv.wait(lock);
	}
	state = State::BUSY;
	
	/*
	LogMessage(Debug, "sending request");
	LogMessage(Debug, "  client id 0x%x", request.client ? request.client->client_id : 0xffffffff);
	LogMessage(Debug, "  object id 0x%x", request.object_id);
	LogMessage(Debug, "  command id 0x%x", request.command_id);
	LogMessage(Debug, "  tag 0x%x", request.tag);
	LogMessage(Debug, "  payload size 0x%lx", request.payload.size());
	*/
	
	mhdr.client_id = request.client ? request.client->client_id : 0xffffffff;
	mhdr.object_id = request.object_id;
	mhdr.command_id = request.command_id;
	mhdr.tag = request.tag;
	mhdr.payload_size = request.payload.size();
	mhdr.object_count = 0;

	request_out = request.Weak();
	pending_requests.push_back(request_out);

	libusb_fill_bulk_transfer(tfer_meta_out, handle, endp_meta_out, (uint8_t*) &mhdr, sizeof(mhdr), &Device::MetaOutTransferShim, SharedPtrForTransfer(), 5000);
	transferring_meta = true;
	int r = libusb_submit_transfer(tfer_meta_out);
	if(r != 0) {
		LogMessage(Debug, "transfer failed: %s", libusb_error_name(r));
		deletion_flag = true;
		return;
	}
}

int USBBackend::Device::GetPriority() {
	return 2; // USB devices are a higher priority than TCP devices
}

std::string USBBackend::Device::GetBridgeType() {
	return "usb";
}

std::shared_ptr<USBBackend::Device> *USBBackend::Device::SharedPtrForTransfer() {
	return new std::shared_ptr<Device>(shared_from_this());
}

void USBBackend::Device::MetaOutTransferCompleted() {
	LogMessage(Debug, "finished transferring meta");
	std::unique_lock<std::mutex> lock(state_mutex);

	if(request_out.payload.size() > 0) {
		LogMessage(Debug, "transferring data");
		libusb_fill_bulk_transfer(tfer_data_out, handle, endp_data_out, (uint8_t*) request_out.payload.data(), LimitTransferSize(request_out.payload.size()), &Device::DataOutTransferShim, SharedPtrForTransfer(), 5000);
		transferring_data = true;
		int r = libusb_submit_transfer(tfer_data_out);
		if(r != 0) {
			LogMessage(Debug, "transfer failed: %s", libusb_error_name(r));
			deletion_flag = true;
			return;
		}
	}

	transferring_meta = false;
	if(!transferring_meta && !transferring_data) {
		LogMessage(Debug, "entering AVAILABLE state");
		state = State::AVAILABLE;
		state_cv.notify_one();
	}
}

void USBBackend::Device::DataOutTransferCompleted() {
	size_t beg_off = (tfer_data_out->buffer - request_out.payload.data());
	size_t read = beg_off + tfer_data_out->actual_length;
	size_t remaining = mhdr.payload_size - read;
	LogMessage(Debug, "send request data 0x%x/0x%x. 0x%x remaining", read, request_out.payload.size(), remaining);

	if(remaining > 0) {
		// continue transferring
		libusb_fill_bulk_transfer(tfer_data_out, handle, endp_data_out,
															request_out.payload.data() + read,
															LimitTransferSize(remaining),
															&Device::DataOutTransferShim, SharedPtrForTransfer(), 15000);
		int r = libusb_submit_transfer(tfer_data_out);
		if(r != 0) {
			LogMessage(Debug, "transfer failed: %s", libusb_error_name(r));
			deletion_flag = true;
			return;
		}
		return;
	} else {
		LogMessage(Debug, "finished transferring data");
		std::unique_lock<std::mutex> lock(state_mutex);
		
		transferring_data = false;
		if(!transferring_meta && !transferring_data) {
			LogMessage(Debug, "entering AVAILABLE state");
			state = State::AVAILABLE;
			state_cv.notify_one();
		}
	}
}

void USBBackend::Device::MetaInTransferCompleted() {
	/*
  LogMessage(Debug, "got response header");
	LogMessage(Debug, "  client id: 0x%x", mhdr_in.client_id);
	LogMessage(Debug, "  object_id: 0x%x", mhdr_in.object_id);
	LogMessage(Debug, "  result_code: 0x%x", mhdr_in.result_code);
	LogMessage(Debug, "  tag: 0x%x", mhdr_in.tag);
	LogMessage(Debug, "  payload_size: 0x%lx", mhdr_in.payload_size);
	LogMessage(Debug, "  object_count: %d", mhdr_in.object_count);
  */

	response_in.device_id = device_id;
	response_in.client_id = mhdr_in.client_id;
	response_in.object_id = mhdr_in.object_id;
	response_in.result_code = mhdr_in.result_code;
	response_in.tag = mhdr_in.tag;
	response_in.payload.resize(mhdr_in.payload_size);
	object_ids_in.resize(mhdr_in.object_count);
	
	if(mhdr_in.payload_size > 0) {
		libusb_fill_bulk_transfer(tfer_data_in, handle, endp_data_in, response_in.payload.data(), LimitTransferSize(response_in.payload.size()), &Device::DataInTransferShim, SharedPtrForTransfer(), 5000);
		int r = libusb_submit_transfer(tfer_data_in);
		if(r != 0) {
			LogMessage(Debug, "transfer failed: %s", libusb_error_name(r));
			deletion_flag = true;
			return;
		}
	} else if(mhdr_in.object_count > 0) {
		libusb_fill_bulk_transfer(tfer_data_in, handle, endp_data_in, (uint8_t*) object_ids_in.data(), object_ids_in.size() * sizeof(uint32_t), &Device::ObjectInTransferShim, SharedPtrForTransfer(), 5000);
		int r = libusb_submit_transfer(tfer_data_in);
		if(r != 0) {
			LogMessage(Debug, "transfer failed: %s", libusb_error_name(r));
			deletion_flag = true;
			return;
		}
	} else {
		DispatchResponse();
	}
}

void USBBackend::Device::DataInTransferCompleted() {
	size_t beg_off = (tfer_data_in->buffer - response_in.payload.data());
	size_t read = beg_off + tfer_data_in->actual_length;
	size_t remaining = mhdr_in.payload_size - read;

	if(remaining > 0) {
		// continue transferring
		libusb_fill_bulk_transfer(tfer_data_in, handle, endp_data_in,
															response_in.payload.data() + read,
															LimitTransferSize(remaining),
															&Device::DataInTransferShim, SharedPtrForTransfer(), 15000);
		int r = libusb_submit_transfer(tfer_data_in);
		if(r != 0) {
			LogMessage(Debug, "transfer failed: %s", libusb_error_name(r));
			deletion_flag = true;
			return;
		}
		return;
	}

	if(mhdr_in.object_count > 0) {
		libusb_fill_bulk_transfer(tfer_data_in, handle, endp_data_in, (uint8_t*) object_ids_in.data(), object_ids_in.size() * sizeof(uint32_t), &Device::ObjectInTransferShim, SharedPtrForTransfer(), 5000);
		int r = libusb_submit_transfer(tfer_data_in);
		if(r != 0) {
			LogMessage(Debug, "transfer failed: %s", libusb_error_name(r));
			deletion_flag = true;
			return;
		}
	} else {
		DispatchResponse();
	}
}

void USBBackend::Device::ObjectInTransferCompleted() {
	if(tfer_data_in->actual_length != mhdr_in.object_count * sizeof(uint32_t)) {
		LogMessage(Debug, "invalid object ID transfer\n");
		deletion_flag = true;
		return;
	}

	DispatchResponse();
}

void USBBackend::Device::DispatchResponse() {
	// create BridgeObjects
	response_in.objects.resize(object_ids_in.size());
	std::transform(
		object_ids_in.begin(), object_ids_in.end(), response_in.objects.begin(),
		[this](uint32_t id) {
			return std::make_shared<BridgeObject>(*backend->twibd, response_in.device_id, id);
		});

	// remove from pending requests
	pending_requests.remove_if([this](WeakRequest &r) {
			return r.tag == response_in.tag;
		});
	
	if(response_in.client_id == 0xFFFFFFFF) { // identification meta-client
		Identified(response_in);
	} else {
		backend->twibd->PostResponse(std::move(response_in));
	}
	ResubmitMetaInTransfer();
}

void USBBackend::Device::Identified(Response &r) {
	LogMessage(Debug, "got identification response back");
	LogMessage(Debug, "payload size: 0x%x", r.payload.size());
	if(r.result_code != 0) {
		LogMessage(Warning, "device identification error: 0x%x", r.result_code);
		deletion_flag = true;
		return;
	}
	std::string err;
	msgpack11::MsgPack obj = msgpack11::MsgPack::parse(std::string(r.payload.begin(), r.payload.end()), err);
	identification = obj;
	device_nickname = obj["device_nickname"].string_value();
	serial_number = obj["serial_number"].string_value();

	LogMessage(Info, "nickname: %s", device_nickname.c_str());
	LogMessage(Info, "serial number: %s", serial_number.c_str());
	
	device_id = std::hash<std::string>()(serial_number);
	LogMessage(Info, "assigned device id: %08x", device_id);
	ready_flag = true;
}

void USBBackend::Device::ResubmitMetaInTransfer() {
	LogMessage(Debug, "submitting meta in transfer");
	libusb_fill_bulk_transfer(tfer_meta_in, handle, endp_meta_in, (uint8_t*) &mhdr_in, sizeof(mhdr_in), &Device::MetaInTransferShim, SharedPtrForTransfer(), 600000);
	int r = libusb_submit_transfer(tfer_meta_in);
	if(r != 0) {
		LogMessage(Debug, "transfer failed: %s", libusb_error_name(r));
		deletion_flag = true;
	}
}

bool USBBackend::Device::CheckTransfer(libusb_transfer *tfer) {
	if(tfer->status != LIBUSB_TRANSFER_COMPLETED) {
		LogMessage(Debug, "transfer failed (status = %d)", tfer->status);
		deletion_flag = true;
		return true;
	} else {
		return false;
	}
}

size_t USBBackend::Device::LimitTransferSize(size_t sz) {
	const size_t max_size = 0x10000;
	if(sz > max_size) {
		return max_size;
	} else {
		return sz;
	}
}

void USBBackend::Device::MetaOutTransferShim(libusb_transfer *tfer) {
	LogMessage(Debug, "meta out transfer shim, status = %d", tfer->status);
	std::shared_ptr<Device> *d = (std::shared_ptr<Device> *) tfer->user_data;
	if(!(*d)->CheckTransfer(tfer)) {
		(*d)->MetaOutTransferCompleted();
	}
	delete d;
}

void USBBackend::Device::DataOutTransferShim(libusb_transfer *tfer) {
	std::shared_ptr<Device> *d = (std::shared_ptr<Device> *) tfer->user_data;
	if(!(*d)->CheckTransfer(tfer)) {
		(*d)->DataOutTransferCompleted();
	}
	delete d;
}

void USBBackend::Device::MetaInTransferShim(libusb_transfer *tfer) {
	std::shared_ptr<Device> *d = (std::shared_ptr<Device> *) tfer->user_data;
	if(tfer->status == LIBUSB_TRANSFER_TIMED_OUT) {
		(*d)->ResubmitMetaInTransfer();
	} else {
		if(!(*d)->CheckTransfer(tfer)) {
			(*d)->MetaInTransferCompleted();
		}
	}
	delete d;
}

void USBBackend::Device::DataInTransferShim(libusb_transfer *tfer) {
	std::shared_ptr<Device> *d = (std::shared_ptr<Device> *) tfer->user_data;
	if(!(*d)->CheckTransfer(tfer)) {
		(*d)->DataInTransferCompleted();
	}
	delete d;
}

void USBBackend::Device::ObjectInTransferShim(libusb_transfer *tfer) {
	std::shared_ptr<Device> *d = (std::shared_ptr<Device> *) tfer->user_data;
	if(!(*d)->CheckTransfer(tfer)) {
		(*d)->ObjectInTransferCompleted();
	}
	delete d;
}

void USBBackend::Probe() {
//#ifdef _WIN32
//	if (TWIBD_HOTPLUG_ENABLED) {
//		KHOT_HANDLE hotHandle = NULL;
//		KHOT_PARAMS hotParams;
//
//		memset(&hotParams, 0, sizeof(hotParams));
//		hotParams.OnHotPlug = hotplug_cb_shim;
//		hotParams.Flags = KHOT_FLAG_PLUG_ALL_ON_INIT;
//		sprintf_s(hotParams.PatternMatch.DeviceID, "*VID_%04X&PID_%04X*", TWILI_VENDOR_ID, TWILI_PRODUCT_ID);
//
//		if (!HotK_Init(&hotHandle, &hotParams)) {
//			LogMessage(Fatal, "failed to register hotplug callback: %d", GetLastError());
//		}
//#else
	if(TWIBD_HOTPLUG_ENABLED && libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
		int r = libusb_hotplug_register_callback(
			ctx,
			(libusb_hotplug_event) (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
			LIBUSB_HOTPLUG_ENUMERATE,
			TWILI_VENDOR_ID, TWILI_PRODUCT_ID,
			LIBUSB_HOTPLUG_MATCH_ANY,
			hotplug_cb_shim, this,
			&hotplug_handle);
		if(r) {
			LogMessage(Fatal, "failed to register hotplug callback: %s", libusb_error_name(r));
		}
		if(TWIBD_ACCEPT_NINTENDO_SDK_DEBUGGER) {
			int r = libusb_hotplug_register_callback(
				ctx,
				(libusb_hotplug_event) (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
				LIBUSB_HOTPLUG_ENUMERATE,
				TWIBD_NINTENDO_SDK_DEBUGGER_VENDOR_ID, TWIBD_NINTENDO_SDK_DEBUGGER_PRODUCT_ID,
				LIBUSB_HOTPLUG_MATCH_ANY,
				hotplug_cb_shim, this,
				&hotplug_handle_nintendo_sdk_debugger);
			if(r) {
				LogMessage(Fatal, "failed to register hotplug callback for nintendo sdk debugger: %s", libusb_error_name(r));
			}
		}
//#endif
	} else {
		LogMessage(Warning, "hotplug is not supported");
		libusb_device **list;
		auto cnt = libusb_get_device_list(NULL, &list);
		if (cnt < 0) {
			LogMessage(Error, "Failed to get device list: %d", cnt);
			exit(1);
		}
		for (auto i = 0; i < cnt; i++) {
			libusb_device *device = list[i];
			libusb_device_descriptor desc;
			if (libusb_get_device_descriptor(device, &desc) < 0) {
				LogMessage(Error, "Failed to get device descriptor");
				exit(1);
			}
			if(desc.idVendor == TWILI_VENDOR_ID && desc.idProduct == TWILI_PRODUCT_ID) {
				this->QueueAddDevice(device);
			}
			if(TWIBD_ACCEPT_NINTENDO_SDK_DEBUGGER) {
				if(desc.idVendor == TWIBD_NINTENDO_SDK_DEBUGGER_VENDOR_ID &&
					 desc.idProduct == TWIBD_NINTENDO_SDK_DEBUGGER_PRODUCT_ID) {
					this->QueueAddDevice(device);
				}
			}
		}
	}
}

void USBBackend::QueueAddDevice(libusb_device *device) {
	//LogMessage(Info, "new device connected, queueing...");
	//libusb_ref_device(device);
	//devices_to_add.push(device);
	AddDevice(device);
}

void USBBackend::AddDevice(libusb_device *device) {
	LogMessage(Info, "probing connected device...");
	struct libusb_device_descriptor descriptor;
	int r = libusb_get_device_descriptor(device, &descriptor);
	if(r != 0) {
		LogMessage(Warning, "failed to get device descriptor: %s", libusb_error_name(r));
		return;
	}

	libusb_config_descriptor *config = NULL;
	r = libusb_get_active_config_descriptor(device, &config);
	if(r != 0) {
		LogMessage(Warning, "failed to get config descriptor: %s", libusb_error_name(r));
		return;
	}

	LogMessage(Debug, "  bNumInterfaces: %d", config->bNumInterfaces);

	const struct libusb_interface_descriptor *twili_interface = NULL;
	for(int i = 0; i < config->bNumInterfaces; i++) {
		const struct libusb_interface *iface = &config->interface[i];
		LogMessage(Debug, "  interface %d:", i);
		LogMessage(Debug, "    num_altsetting: %d", iface->num_altsetting);
		for(int j = 0; j < iface->num_altsetting; j++) {
			const struct libusb_interface_descriptor *altsetting = &iface->altsetting[j];
			LogMessage(Debug, "    altsetting %d:", j);
			LogMessage(Debug, "      bNumEndpoints: %d", altsetting->bNumEndpoints);
			LogMessage(Debug, "      bInterfaceClass: 0x%x", altsetting->bInterfaceClass);
			LogMessage(Debug, "      bInterfaceSubClass: 0x%x", altsetting->bInterfaceSubClass);
			LogMessage(Debug, "      bInterfaceProtocol: 0x%x", altsetting->bInterfaceProtocol);
			if(altsetting->bInterfaceClass == 0xFF &&
				 altsetting->bInterfaceSubClass == 0x1 &&
				 altsetting->bInterfaceProtocol == 0x0) { // twili interface
				if(twili_interface != NULL) {
					LogMessage(Warning, "    device has multiple twili interfaces?");
				}
				twili_interface = altsetting;
				break;
			}
			if(altsetting->bInterfaceClass == 0xFF &&
				 altsetting->bInterfaceSubClass == 0x0 &&
				 altsetting->bInterfaceProtocol == 0x0) { // stdio console interface
				ProbeStdioInterface(device, altsetting);
			}
		}
	}

	if(twili_interface == NULL) {
		LogMessage(Info, "could not find Twili interface");
		libusb_free_config_descriptor(config);
		return;
	}

	if(twili_interface->bNumEndpoints != 4) {
		LogMessage(Warning, "Twili interface exposes a bad number of endpoints");
		libusb_free_config_descriptor(config);
		return;
	}
	
	libusb_endpoint_descriptor endp_meta_out = twili_interface->endpoint[0];
	libusb_endpoint_descriptor endp_data_out = twili_interface->endpoint[1];
	libusb_endpoint_descriptor endp_meta_in = twili_interface->endpoint[2];
	libusb_endpoint_descriptor endp_data_in = twili_interface->endpoint[3];

	if((endp_meta_out.bEndpointAddress & 0x80) != LIBUSB_ENDPOINT_OUT ||
		 (endp_data_out.bEndpointAddress & 0x80) != LIBUSB_ENDPOINT_OUT ||
		 (endp_meta_in.bEndpointAddress & 0x80) != LIBUSB_ENDPOINT_IN ||
		 (endp_data_in.bEndpointAddress & 0x80) != LIBUSB_ENDPOINT_IN) {
		LogMessage(Warning, "Twili interface exposes endpoints with bad directions");
		libusb_free_config_descriptor(config);
		return;
	}

	if((endp_meta_out.bmAttributes & 0x3) != LIBUSB_TRANSFER_TYPE_BULK ||
		 (endp_data_out.bmAttributes & 0x3) != LIBUSB_TRANSFER_TYPE_BULK ||
		 (endp_meta_in.bmAttributes & 0x3) != LIBUSB_TRANSFER_TYPE_BULK ||
		 (endp_data_in.bmAttributes & 0x3) != LIBUSB_TRANSFER_TYPE_BULK) {
		LogMessage(Warning, "Twili interface exposes endpoints with bad transfer types");
		libusb_free_config_descriptor(config);
		return;
	}

	libusb_device_handle *handle;
	r = libusb_open(device, &handle);
	if(r != 0) {
		LogMessage(Warning, "failed to open device: %s", libusb_error_name(r));
		libusb_free_config_descriptor(config);
		return;
	}
	libusb_set_auto_detach_kernel_driver(handle, true);
	r = libusb_claim_interface(handle, twili_interface->bInterfaceNumber);
	if(r != 0) {
		LogMessage(Warning, "failed to claim interface: %s", libusb_error_name(r));
		libusb_close(handle);
		libusb_free_config_descriptor(config);
	}
	
	uint8_t addrs[] = {
		endp_meta_out.bEndpointAddress,
		endp_data_out.bEndpointAddress,
		endp_meta_in.bEndpointAddress,
		endp_data_in.bEndpointAddress};
	
	devices.emplace_back(std::make_shared<Device>(this, handle, addrs, twili_interface->bInterfaceNumber))->Begin();
	
	libusb_free_config_descriptor(config);
}

void USBBackend::RemoveDevice(libusb_context *ctx, libusb_device *device) {
	LogMessage(Debug, "a device was removed, but not sure which one");
}

void USBBackend::ProbeStdioInterface(libusb_device *device, const libusb_interface_descriptor *d) {
	LogMessage(Debug, "probing stdio interface");
	if(d->bNumEndpoints != 2) {
		LogMessage(Warning, "Stdio interface exposes a bad number of endpoints");
		return;
	}
	libusb_endpoint_descriptor endp_stdio_out = d->endpoint[0];
	libusb_endpoint_descriptor endp_stdio_in = d->endpoint[1];
	if((endp_stdio_out.bEndpointAddress & 0x80) != LIBUSB_ENDPOINT_OUT ||
		 (endp_stdio_in.bEndpointAddress  & 0x80) != LIBUSB_ENDPOINT_IN) {
		LogMessage(Warning, "Stdio interface exposes endpoints with bad directions");
		return;
	}

	if((endp_stdio_out.bmAttributes & 0x3) != LIBUSB_TRANSFER_TYPE_BULK ||
		 (endp_stdio_in.bmAttributes  & 0x3) != LIBUSB_TRANSFER_TYPE_BULK) {
		LogMessage(Warning, "Stdio interface exposes endpoints with bad transfer types");
		return;
	}
	
	libusb_device_handle *handle;
	int r = libusb_open(device, &handle);
	if(r != 0) {
		LogMessage(Warning, "failed to open device: %s", libusb_error_name(r));
		return;
	}
	libusb_set_auto_detach_kernel_driver(handle, true);

	r = libusb_claim_interface(handle, d->bInterfaceNumber);
	if(r != 0) {
		LogMessage(Warning, "failed to claim interface: %s", libusb_error_name(r));
		libusb_close(handle);
		return;
	}

	auto state = std::make_shared<StdoutTransferState>(handle, endp_stdio_in.bEndpointAddress);
	stdout_transfers.push_back(state);
	state->Submit();
}

USBBackend::StdoutTransferState::StdoutTransferState(libusb_device_handle *handle, uint8_t addr) :
	handle(handle), address(addr) {
	tfer = libusb_alloc_transfer(0);
}

USBBackend::StdoutTransferState::~StdoutTransferState() {
	libusb_free_transfer(tfer);
	libusb_close(handle);
}

void USBBackend::StdoutTransferState::Submit() {
	libusb_fill_bulk_transfer(tfer, handle, address, io_buffer, sizeof(io_buffer), &StdoutTransferState::Callback, this, 60000);
	int r = libusb_submit_transfer(tfer);
	if(r != 0) {
		LogMessage(Debug, "submit failed");
		Kill();
	}
}

void USBBackend::StdoutTransferState::Kill() {
	deletion_flag = true;
}

void USBBackend::StdoutTransferState::Callback(libusb_transfer *tfer) {
	StdoutTransferState *state = (StdoutTransferState*) tfer->user_data;
	if(tfer->status == LIBUSB_TRANSFER_COMPLETED) {
		state->string_buffer.Write(state->io_buffer, tfer->actual_length);
		for(bool found_line = true; found_line; found_line = false) {
			char *current_line = (char*) state->string_buffer.Read();
			for(size_t i = 0; i < state->string_buffer.ReadAvailable(); i++) {
				if(current_line[i] == '\n') {
					LogMessage(Message, "[TWILI] %.*s", i, current_line);
					state->string_buffer.MarkRead(i+1);
					found_line = true;
					break;
				}
			}
		}
		state->Submit();
	} else if(tfer->status == LIBUSB_TRANSFER_TIMED_OUT) {
		state->Submit();
	} else {
		LogMessage(Debug, "stdout transfer failed");
		state->Kill();
	}
}

void USBBackend::event_thread_func() {
	while(!event_thread_destroy) {
		libusb_handle_events(ctx);

		while(!devices_to_add.empty()) {
			libusb_device *d = devices_to_add.front();
			devices_to_add.pop();
			AddDevice(d);
			libusb_unref_device(d);
		}
		
		for(auto i = devices.begin(); i != devices.end(); ) {
			auto d = *i;
			if(d->ready_flag && !d->added_flag) {
				twibd->AddDevice(d);
				d->added_flag = true;
			}
			
			if(d->deletion_flag) {
				if(d->added_flag) {
					twibd->RemoveDevice(d);
				}
				i = devices.erase(i);
			} else {
				i++;
			}
		}

		for(auto i = stdout_transfers.begin(); i != stdout_transfers.end(); ) {
			if((*i)->deletion_flag) {
				i = stdout_transfers.erase(i);
			} else {
				i++;
			}
		}
	}
}

} // namespace backend
} // namespace twibd
} // namespace twili
