#include "USBBackend.hpp"

#include<msgpack11.hpp>

#include "Twibd.hpp"
#include "config.hpp"
#include "err.hpp"

#ifdef _WIN32
#include<libusbk.h>
#endif

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
//			log(ERR, "Failed to get device list: %d", cnt);
//			exit(1);
//		}
//		for (auto i = 0; i < cnt; i++) {
//			libusb_device *device = list[i];
//			libusb_device_descriptor desc;
//			if (libusb_get_device_descriptor(device, &desc) < 0) {
//				log(ERR, "Failed to get device descriptor");
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
		log(FATAL, "failed to initialize libusb: %s", libusb_error_name(r));
		exit(1);
	}

	std::thread event_thread(&USBBackend::event_thread_func, this);
	this->event_thread = std::move(event_thread);
}

USBBackend::~USBBackend() {
	event_thread_destroy = true;
	if(Twibd_HOTPLUG_ENABLED && libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
		libusb_hotplug_deregister_callback(ctx, hotplug_handle); // wakes up usb_event_thread
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
	SendRequest(Request(0xFFFFFFFF, 0x0, 0x0, (uint32_t) protocol::ITwibDeviceInterface::Command::IDENTIFY, 0xFFFFFFFF, std::vector<uint8_t>()));
}

void USBBackend::Device::SendRequest(const Request &&request) {
	std::unique_lock<std::mutex> lock(state_mutex);
	while(state != State::AVAILABLE) {
		state_cv.wait(lock);
	}
	state = State::BUSY;
	
	log(DEBUG, "sending request");
	log(DEBUG, "  client id 0x%x", request.client_id);
	log(DEBUG, "  object id 0x%x", request.object_id);
	log(DEBUG, "  command id 0x%x", request.command_id);
	log(DEBUG, "  tag 0x%x", request.tag);
	log(DEBUG, "  payload size 0x%lx", request.payload.size());
	
	mhdr.client_id = request.client_id;
	mhdr.object_id = request.object_id;
	mhdr.command_id = request.command_id;
	mhdr.tag = request.tag;
	mhdr.payload_size = request.payload.size();

	pending_requests.push_back(request);

	request_out = request;
	libusb_fill_bulk_transfer(tfer_meta_out, handle, endp_meta_out, (uint8_t*) &mhdr, sizeof(mhdr), &Device::MetaOutTransferShim, SharedPtrForTransfer(), 5000);
	transferring_meta = true;
	int r = libusb_submit_transfer(tfer_meta_out);
	if(r != 0) {
		log(DEBUG, "transfer failed: %s", libusb_error_name(r));
		deletion_flag = true;
		return;
	}
}

std::shared_ptr<USBBackend::Device> *USBBackend::Device::SharedPtrForTransfer() {
	return new std::shared_ptr<Device>(shared_from_this());
}

void USBBackend::Device::MetaOutTransferCompleted() {
	log(DEBUG, "finished transferring meta");
	std::unique_lock<std::mutex> lock(state_mutex);

	if(request_out.payload.size() > 0) {
		log(DEBUG, "transferring data");
		libusb_fill_bulk_transfer(tfer_data_out, handle, endp_data_out, (uint8_t*) request_out.payload.data(), LimitTransferSize(request_out.payload.size()), &Device::DataOutTransferShim, SharedPtrForTransfer(), 5000);
		transferring_data = true;
		int r = libusb_submit_transfer(tfer_data_out);
		if(r != 0) {
			log(DEBUG, "transfer failed: %s", libusb_error_name(r));
			deletion_flag = true;
			return;
		}
	}
	
	transferring_meta = false;
	if(!transferring_meta && !transferring_data) {
		log(DEBUG, "entering AVAILABLE state");
		state = State::AVAILABLE;
	}
}

void USBBackend::Device::DataOutTransferCompleted() {
	size_t beg_off = (tfer_data_out->buffer - request_out.payload.data());
	size_t read = beg_off + tfer_data_out->actual_length;
	size_t remaining = mhdr.payload_size - read;
	log(DEBUG, "send request data 0x%x/0x%x. 0x%x remaining", read, request_out.payload.size(), remaining);

	if(remaining > 0) {
		// continue transferring
		libusb_fill_bulk_transfer(tfer_data_out, handle, endp_data_out,
															request_out.payload.data() + read,
															LimitTransferSize(remaining),
															&Device::DataOutTransferShim, SharedPtrForTransfer(), 15000);
		int r = libusb_submit_transfer(tfer_data_out);
		if(r != 0) {
			log(DEBUG, "transfer failed: %s", libusb_error_name(r));
			deletion_flag = true;
			return;
		}
		return;
	} else {
		log(DEBUG, "finished transferring data");
		std::unique_lock<std::mutex> lock(state_mutex);
		
		transferring_data = false;
		if(!transferring_meta && !transferring_data) {
			log(DEBUG, "entering AVAILABLE state");
			state = State::AVAILABLE;
		}
	}
}

void USBBackend::Device::MetaInTransferCompleted() {
	log(DEBUG, "got response header");
	log(DEBUG, "  client id: 0x%x", mhdr_in.client_id);
	log(DEBUG, "  object_id: 0x%x", mhdr_in.object_id);
	log(DEBUG, "  result_code: 0x%x", mhdr_in.result_code);
	log(DEBUG, "  tag: 0x%x", mhdr_in.tag);
	log(DEBUG, "  payload_size: 0x%lx", mhdr_in.payload_size);

	response_in.device_id = device_id;
	response_in.client_id = mhdr_in.client_id;
	response_in.object_id = mhdr_in.object_id;
	response_in.result_code = mhdr_in.result_code;
	response_in.tag = mhdr_in.tag;
	response_in.payload.resize(mhdr_in.payload_size);
	log(DEBUG, "  payload.size(): 0x%lx", response_in.payload.size());
	
	if(mhdr_in.payload_size > 0) {
		libusb_fill_bulk_transfer(tfer_data_in, handle, endp_data_in, response_in.payload.data(), LimitTransferSize(response_in.payload.size()), &Device::DataInTransferShim, SharedPtrForTransfer(), 5000);
		int r = libusb_submit_transfer(tfer_data_in);
		if(r != 0) {
			log(DEBUG, "transfer failed: %s", libusb_error_name(r));
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
	log(DEBUG, "got response data 0x%x/0x%x, 0x%x remaining", read, response_in.payload.size(), remaining);
	
	if(remaining > 0) {
		// continue transferring
		libusb_fill_bulk_transfer(tfer_data_in, handle, endp_data_in,
															response_in.payload.data() + read,
															LimitTransferSize(remaining),
															&Device::DataInTransferShim, SharedPtrForTransfer(), 15000);
		int r = libusb_submit_transfer(tfer_data_in);
		if(r != 0) {
			log(DEBUG, "transfer failed: %s", libusb_error_name(r));
			deletion_flag = true;
			return;
		}
		return;
	}

	DispatchResponse();
}

void USBBackend::Device::DispatchResponse() {
	pending_requests.remove_if([this](Request &r) {
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
	log(DEBUG, "got identification response back");
	log(DEBUG, "payload size: 0x%x", r.payload.size());
	if(r.result_code != 0) {
		log(WARN, "device identification error: 0x%x", r.result_code);
		deletion_flag = true;
	}
	std::string err;
	msgpack11::MsgPack obj = msgpack11::MsgPack::parse(std::string(r.payload.begin(), r.payload.end()), err);
	identification = obj;
	device_nickname = obj["device_nickname"].string_value();
	std::vector<uint8_t> sn = obj["serial_number"].binary_items();
	serial_number = std::string(sn.begin(), sn.end());

	log(INFO, "nickname: %s", device_nickname.c_str());
	log(INFO, "serial number: %s", serial_number.c_str());
	
	device_id = std::hash<std::string>()(serial_number);
	log(INFO, "assigned device id: %08x", device_id);
	ready_flag = true;
}

void USBBackend::Device::ResubmitMetaInTransfer() {
	log(DEBUG, "submitting meta in transfer");
	libusb_fill_bulk_transfer(tfer_meta_in, handle, endp_meta_in, (uint8_t*) &mhdr_in, sizeof(mhdr_in), &Device::MetaInTransferShim, SharedPtrForTransfer(), 5000);
	int r = libusb_submit_transfer(tfer_meta_in);
	if(r != 0) {
		log(DEBUG, "transfer failed: %s", libusb_error_name(r));
		deletion_flag = true;
	}
}

bool USBBackend::Device::CheckTransfer(libusb_transfer *tfer) {
	if(tfer->status != LIBUSB_TRANSFER_COMPLETED) {
		log(DEBUG, "transfer failed (status = %d)", tfer->status);
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
	log(DEBUG, "meta out transfer shim, status = %d", tfer->status);
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
	log(DEBUG, "data in transfer shim, status = %d", tfer->status);
	std::shared_ptr<Device> *d = (std::shared_ptr<Device> *) tfer->user_data;
	if(!(*d)->CheckTransfer(tfer)) {
		(*d)->DataInTransferCompleted();
	}
	delete d;
}

void USBBackend::Probe() {
//#ifdef _WIN32
//	if (Twibd_HOTPLUG_ENABLED) {
//		KHOT_HANDLE hotHandle = NULL;
//		KHOT_PARAMS hotParams;
//
//		memset(&hotParams, 0, sizeof(hotParams));
//		hotParams.OnHotPlug = hotplug_cb_shim;
//		hotParams.Flags = KHOT_FLAG_PLUG_ALL_ON_INIT;
//		sprintf_s(hotParams.PatternMatch.DeviceID, "*VID_%04X&PID_%04X*", Twili_VENDOR_ID, Twili_PRODUCT_ID);
//
//		if (!HotK_Init(&hotHandle, &hotParams)) {
//			log(FATAL, "failed to register hotplug callback: %d", GetLastError());
//		}
//#else
	if(Twibd_HOTPLUG_ENABLED && libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
		int r = libusb_hotplug_register_callback(
			ctx,
			(libusb_hotplug_event) (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
			LIBUSB_HOTPLUG_ENUMERATE,
			Twili_VENDOR_ID, Twili_PRODUCT_ID,
			LIBUSB_HOTPLUG_MATCH_ANY,
			hotplug_cb_shim, this,
			&hotplug_handle);
		if(r) {
			log(FATAL, "failed to register hotplug callback: %s", libusb_error_name(r));
		}
//#endif
	} else {
		log(WARN, "hotplug is not supported");
		libusb_device **list;
		auto cnt = libusb_get_device_list(NULL, &list);
		if (cnt < 0) {
			log(ERR, "Failed to get device list: %d", cnt);
			exit(1);
		}
		for (auto i = 0; i < cnt; i++) {
			libusb_device *device = list[i];
			libusb_device_descriptor desc;
			if (libusb_get_device_descriptor(device, &desc) < 0) {
				log(ERR, "Failed to get device descriptor");
				exit(1);
			}
			if (desc.idVendor == Twili_VENDOR_ID && desc.idProduct == Twili_PRODUCT_ID) {
				this->QueueAddDevice(device);
			}
		}
	}
}

void USBBackend::QueueAddDevice(libusb_device *device) {
	//log(INFO, "new device connected, queueing...");
	//libusb_ref_device(device);
	//devices_to_add.push(device);
	AddDevice(device);
}

void USBBackend::AddDevice(libusb_device *device) {
	log(INFO, "probing connected device...");
	struct libusb_device_descriptor descriptor;
	int r = libusb_get_device_descriptor(device, &descriptor);
	if(r != 0) {
		log(WARN, "failed to get device descriptor: %s", libusb_error_name(r));
		return;
	}

	libusb_config_descriptor *config = NULL;
	r = libusb_get_active_config_descriptor(device, &config);
	if(r != 0) {
		log(WARN, "failed to get config descriptor: %s", libusb_error_name(r));
		return;
	}

	log(DEBUG, "  bNumInterfaces: %d", config->bNumInterfaces);

	const struct libusb_interface_descriptor *twili_interface = NULL;
	for(int i = 0; i < config->bNumInterfaces && twili_interface == NULL; i++) {
		const struct libusb_interface *iface = &config->interface[i];
		log(DEBUG, "  interface %d:", i);
		log(DEBUG, "    num_altsetting: %d", iface->num_altsetting);
		for(int j = 0; j < iface->num_altsetting; j++) {
			const struct libusb_interface_descriptor *altsetting = &iface->altsetting[j];
			log(DEBUG, "    altsetting %d:", j);
			log(DEBUG, "      bNumEndpoints: %d", altsetting->bNumEndpoints);
			log(DEBUG, "      bInterfaceClass: 0x%x", altsetting->bInterfaceClass);
			log(DEBUG, "      bInterfaceSubClass: 0x%x", altsetting->bInterfaceSubClass);
			log(DEBUG, "      bInterfaceProtocol: 0x%x", altsetting->bInterfaceProtocol);
			if(altsetting->bInterfaceClass == 0xFF &&
				 altsetting->bInterfaceSubClass == 0x1 &&
				 altsetting->bInterfaceProtocol == 0x0) {
				twili_interface = altsetting;
				break;
			}
		}
	}

	if(twili_interface == NULL) {
		log(INFO, "could not find Twili interface");
		libusb_free_config_descriptor(config);
		return;
	}

	if(twili_interface->bNumEndpoints != 4) {
		log(WARN, "Twili interface exposes a bad number of endpoints");
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
		log(WARN, "Twili interface exposes endpoints with bad directions");
		libusb_free_config_descriptor(config);
		return;
	}

	if((endp_meta_out.bmAttributes & 0x3) != LIBUSB_TRANSFER_TYPE_BULK ||
		 (endp_data_out.bmAttributes & 0x3) != LIBUSB_TRANSFER_TYPE_BULK ||
		 (endp_meta_in.bmAttributes & 0x3) != LIBUSB_TRANSFER_TYPE_BULK ||
		 (endp_data_in.bmAttributes & 0x3) != LIBUSB_TRANSFER_TYPE_BULK) {
		log(WARN, "Twili interface exposes endpoints with bad transfer types");
		libusb_free_config_descriptor(config);
		return;
	}

	libusb_device_handle *handle;
	r = libusb_open(device, &handle);
	if(r != 0) {
		log(WARN, "failed to open device: %s", libusb_error_name(r));
		libusb_free_config_descriptor(config);
		return;
	}
	libusb_set_auto_detach_kernel_driver(handle, true);
	r = libusb_claim_interface(handle, twili_interface->bInterfaceNumber);
	if(r != 0) {
		log(WARN, "failed to claim interface: %s", libusb_error_name(r));
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
	log(DEBUG, "a device was removed, but not sure which one");
}

void USBBackend::event_thread_func() {
	while(!event_thread_destroy) {
		log(DEBUG, "usb event thread loop");
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
				twibd->RemoveDevice(d);
				i = devices.erase(i);
			} else {
				i++;
			}
		}
	}
}

} // namespace backend
} // namespace twibd
} // namespace twili
