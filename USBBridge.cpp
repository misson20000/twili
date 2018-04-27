typedef bool _Bool;
#include<atomic>
#include<memory>
#include<stdint.h>
#include<stdatomic.h>
#include<libtransistor/alloc_pages.h>
#include<libtransistor/err.h>
#include<libtransistor/util.h>
#include<libtransistor/svc.h>

#include "twili.hpp"
#include "USBBridge.hpp"

namespace twili {
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
	.bNumEndpoints = 0x00,
	.bInterfaceClass = 0xFF,
	.bInterfaceSubClass = 0x01,
	.bInterfaceProtocol = 0x00,
	.iInterface = 0x00
};

const size_t TRANSFER_BUFFER_SIZE = 0x8000;

using Transistor::ResultCode;
using Transistor::ResultError;

USBBridge::USBBridge(Twili *twili) :
	twili(twili),
	ds(ResultCode::AssertOk(Transistor::IPC::USB::DS::DS::Initialize(2))),
	usb_state_change_event(ResultCode::AssertOk(ds.GetStateChangeEvent())) {
	
	interface = ResultCode::AssertOk(
		ds.GetInterface(interface_descriptor, "twili_bridge"));
	endpoint_in = ResultCode::AssertOk(
		interface->GetEndpoint(endpoint_in_descriptor));
	endpoint_out = ResultCode::AssertOk(
		interface->GetEndpoint(endpoint_out_descriptor));
	ResultCode::AssertOk(interface->Enable());

	incoming_buffer = (uint8_t*) alloc_pages(TRANSFER_BUFFER_SIZE, TRANSFER_BUFFER_SIZE, nullptr);
	if(incoming_buffer == NULL) {
		throw new ResultError(LIBTRANSISTOR_ERR_OUT_OF_MEMORY);
	}
	auto r = ResultCode::ExpectOk(svcSetMemoryAttribute(incoming_buffer, TRANSFER_BUFFER_SIZE, 0x8, 0x8));
	if(!r) {
		free_pages(incoming_buffer);
		throw new ResultError(r.error());
	}

	outgoing_buffer = (uint8_t*) alloc_pages(TRANSFER_BUFFER_SIZE, TRANSFER_BUFFER_SIZE, nullptr);
	if(outgoing_buffer == NULL) {
		svcSetMemoryAttribute(incoming_buffer, TRANSFER_BUFFER_SIZE, 0x0, 0x0);
		free_pages(incoming_buffer);
		throw new ResultError(LIBTRANSISTOR_ERR_OUT_OF_MEMORY);
	}
	r = ResultCode::ExpectOk(svcSetMemoryAttribute(outgoing_buffer, TRANSFER_BUFFER_SIZE, 0x8, 0x8));
	if(!r) {
		free_pages(outgoing_buffer);
		svcSetMemoryAttribute(incoming_buffer, TRANSFER_BUFFER_SIZE, 0x0, 0x0);
		free_pages(incoming_buffer);
		throw new ResultError(r.error());
	}

	// this should be synchronous
	while(USBStateChangeCallback()) {
		usb_state_change_event.ResetSignal();
		usb_state_change_event.WaitSignal(100000000);
	}

	endpoint_out_completion_wait = twili->event_waiter.Add(endpoint_out->completion_event, [this]() {
			endpoint_out->completion_event.ResetSignal();
			USBTransactionCompleted();
			return state != State::INVALID;
		});
	
	BeginReadHeader();
}

void USBBridge::USBTransactionCompleted() {
	usb_ds_report_t report = ResultCode::AssertOk(endpoint_out->GetReportData());
	usb_ds_report_entry_t *entry = nullptr;
	for(uint32_t i = 0; i < report.entry_count; i++) {
		if(report.entries[i].urb_id == recv_urb_id) {
			entry = &report.entries[i];
		}
	}
	if(entry == nullptr) {
		printf("could not find URB entry\n");
		state = State::INVALID;
		return;
	}
	if(entry->urb_status != 3) {
		printf("URB status (%d) != 3\n", entry->urb_status);
		if(state == State::WAITING_ON_HEADER) {
			BeginReadHeader();
		} else {
			state = State::INVALID;
		}
		return;
	}
	
	switch(state) {
	case State::WAITING_ON_HEADER: {
		printf("was waiting for command header...\n");
		if(entry->transferred_size != sizeof(CommandHeader)) {
			printf("URB transferred_size (0x%x) != sizeof(CommandHeader)\n", entry->transferred_size);
			if(entry->transferred_size == 0) {
				printf("forgiving an empty transfer...\n");
				BeginReadHeader();
				return;
			} else {
				state = State::INVALID;
				return;
			}
		}
		memcpy(&current_header, incoming_buffer, sizeof(current_header));
		printf("got header, command %ld, payload size 0x%lx\n", current_header.command_id, current_header.payload_size);
		if(!ValidateCommandHeader()) {
			printf("bad command header\n");
			state = State::INVALID;
			return;
		}
		BeginReadPayload();
		break;
	}
	case State::WAITING_ON_PAYLOAD: {
		if(current_payload.size() + entry->transferred_size > current_header.payload_size) {
			printf("overshot payload size\n");
			state = State::INVALID;
			return;
		}
		std::copy(incoming_buffer, incoming_buffer + entry->transferred_size, current_payload.insert(current_payload.end(), entry->transferred_size, 0));
		if(current_payload.size() < current_header.payload_size) {
			ContinueReadingPayload();
		} else {
			printf("got entire payload\n");
			if(!ProcessCommand()) {
				printf("failed to process command\n");
				state = State::INVALID;
				return;
			}
			BeginReadHeader();
		}
		break;
	}
	case State::INVALID:
		printf("transaction completed on invalid state\n");
		break;
	}
}

bool USBBridge::USBStateChangeCallback() {
	//printf("USB state change signalled\n");
	if(ResultCode::AssertOk(ds.GetState()) == Transistor::IPC::USB::DS::State::INITIALIZED) {
		printf("finished USB bringup\n");
		return false;
	} else {
		return true;
	}
}

void USBBridge::BeginReadHeader() {
	printf("BeginReadHeader\n");
	state = State::WAITING_ON_HEADER;
	recv_urb_id = ResultCode::AssertOk(endpoint_out->PostBufferAsync(incoming_buffer, sizeof(CommandHeader)));
	printf("  => URB 0x%x\n", recv_urb_id);
}

void USBBridge::BeginReadPayload() {
	state = State::WAITING_ON_PAYLOAD;
	current_payload.clear();
	current_payload.reserve(current_header.payload_size);
	ContinueReadingPayload();
}

void USBBridge::ContinueReadingPayload() {
	size_t size = TRANSFER_BUFFER_SIZE;
	if(current_header.payload_size - current_payload.size() < size) {
		size = current_header.payload_size - current_payload.size();
	}
	recv_urb_id = ResultCode::AssertOk(endpoint_out->PostBufferAsync(incoming_buffer, size));
}

bool USBBridge::ValidateCommandHeader() {
	switch((CommandID) current_header.command_id) {
	case CommandID::RUN:
		return true; // any payload size is ok
	case CommandID::REBOOT:
		return true; // any payload size is ok
	default:
		return false;
	}
	return true;
}

bool USBBridge::ProcessCommand() {
	switch((CommandID) current_header.command_id) {
	case CommandID::RUN:
		return twili->Run(current_payload);
	case CommandID::REBOOT:
		return twili->Reboot();
	}
	return false;
}

USBBridge::~USBBridge() {
	free_pages(incoming_buffer);
	free_pages(outgoing_buffer);
}

}
}
