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

#include "USBBridge.hpp"

typedef bool _Bool;
#include<atomic>
#include<memory>
#include<stdint.h>
#include<stdatomic.h>
#include<libtransistor/alloc_pages.h>
#include<libtransistor/err.h>
#include<libtransistor/util.h>
#include<libtransistor/svc.h>

#include "../../twili.hpp"
#include "../Object.hpp"

#include "err.hpp"

namespace twili {
namespace bridge {
namespace usb {

static usb_endpoint_descriptor_t endpoint_in_descriptor = {
	.bLength = sizeof(usb_endpoint_descriptor_t),
	.bDescriptorType = TRN_USB_DT_ENDPOINT,
	.bEndpointAddress = TRN_USB_ENDPOINT_IN,
	.bmAttributes = TRN_USB_TRANSFER_TYPE_BULK,
	.wMaxPacketSize = 0x200,
	.bInterval = 0,
};

static usb_endpoint_descriptor_t endpoint_out_descriptor = {
	.bLength = sizeof(usb_endpoint_descriptor_t),
	.bDescriptorType = TRN_USB_DT_ENDPOINT,
	.bEndpointAddress = TRN_USB_ENDPOINT_OUT,
	.bmAttributes = TRN_USB_TRANSFER_TYPE_BULK,
	.wMaxPacketSize = 0x200,
	.bInterval = 0,
};

static usb_interface_descriptor_t interface_descriptor = {
	.bLength = 0x9,
	.bDescriptorType = TRN_USB_DT_INTERFACE,
	.bInterfaceNumber = USB_DS_INTERFACE_NUMBER_AUTO,
	.bAlternateSetting = 0x00,
	.bNumEndpoints = 0x04,
	.bInterfaceClass = 0xFF,
	.bInterfaceSubClass = 0x01,
	.bInterfaceProtocol = 0x00,
	.iInterface = 0x00
};

using trn::ResultCode;
using trn::ResultError;

static const size_t TRANSFER_BUFFER_SIZE = 0x8000;

USBBridge::USBBridge(Twili *twili, std::shared_ptr<bridge::Object> object_zero) :
	twili(twili),
	ds(ResultCode::AssertOk(trn::service::usb::ds::DS::Initialize(2, nullptr))),
	usb_state_change_event(ResultCode::AssertOk(ds.GetStateChangeEvent())),
	request_reader(this),
	request_meta_buffer(0x1000),
	response_meta_buffer(0x1000),
	request_data_buffer(TRANSFER_BUFFER_SIZE),
	response_data_buffer(TRANSFER_BUFFER_SIZE) {
	
	interface = ResultCode::AssertOk(
		ds.GetInterface(interface_descriptor, "twili_bridge"));
	endpoint_request_meta = ResultCode::AssertOk(
		interface->GetEndpoint(endpoint_out_descriptor)); // out from host
	endpoint_request_data = ResultCode::AssertOk(
		interface->GetEndpoint(endpoint_out_descriptor)); // out from host
	endpoint_response_meta = ResultCode::AssertOk(
		interface->GetEndpoint(endpoint_in_descriptor)); // into host
	endpoint_response_data = ResultCode::AssertOk(
		interface->GetEndpoint(endpoint_in_descriptor)); // into host
	ResultCode::AssertOk(interface->Enable());

	objects.insert(std::pair<uint32_t, std::shared_ptr<bridge::Object>>(0, object_zero));

	state_change_wait = twili->event_waiter.Add(usb_state_change_event, [this]() {
			return USBStateChangeCallback();
		});
}

usb_ds_report_entry_t *USBBridge::FindReport(std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint, usb_ds_report_t &report, uint32_t urb_id) {
	report = ResultCode::AssertOk(endpoint->GetReportData());
	usb_ds_report_entry_t *entry = nullptr;
	for(uint32_t i = 0; i < report.entry_count; i++) {
		if(report.entries[i].urb_id == urb_id) {
			return &report.entries[i];
		}
	}
	throw ResultError(TWILI_ERR_FATAL_USB_TRANSFER);
}

void USBBridge::PostBufferSync(std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint, uint8_t *buffer, size_t size) {
	uint32_t urb_id = ResultCode::AssertOk(endpoint->PostBufferAsync(buffer, size));
	ResultCode::AssertOk(endpoint->completion_event.WaitSignal(30000000000));
	ResultCode::AssertOk(endpoint->completion_event.ResetSignal());
	
	usb_ds_report_t report;
	auto entry = FindReport(endpoint, report, urb_id);
	if(entry->urb_status != 3) {
		throw ResultError(TWILI_ERR_USB_TRANSFER);
	}
	if(entry->transferred_size < size) {
		printf("[USBB] didn't send all bytes, posting again...\n");
		PostBufferSync(endpoint, buffer + entry->transferred_size, size - entry->transferred_size);
	}
}

bool USBBridge::USBStateChangeCallback() {
	if(ResultCode::AssertOk(ds.GetState()) == trn::service::usb::ds::State::INITIALIZED) {
		printf("finished USB bringup\n");
		try {
			request_reader.Begin();
		} catch(ResultError &e) { // until libtransistor 2.0.1, we have to live with ResultError not inheriting publicly from std::runtime_error
			ResetInterface();
		} catch(std::exception &e) {
			ResetInterface();
		}
	} else {
		// USB has gone down
	}
	return true;
}

void USBBridge::ResetInterface() {
	interface->Disable();
	interface->Enable();
}

USBBridge::~USBBridge() {
}

USBBuffer::USBBuffer(size_t size) : size(size) {
	data = (uint8_t*) alloc_pages(size, size, nullptr);
	if(data == NULL) {
		throw ResultError(LIBTRANSISTOR_ERR_OUT_OF_MEMORY);
	}
	auto r = ResultCode::ExpectOk(svcSetMemoryAttribute(data, size, 0x8, 0x8));
	if(!r) {
		free_pages(data);
		throw ResultError(r.error());
	}
}

USBBuffer::~USBBuffer() {
	ResultCode::AssertOk(svcSetMemoryAttribute(data, size, 0x0, 0x0));
	free_pages(data);
}

} // namespace usb
} // namespace bridge
} // namespace twili
