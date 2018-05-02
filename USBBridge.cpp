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
#include "err.hpp"

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

using trn::ResultCode;
using trn::ResultError;

static const size_t TRANSFER_BUFFER_SIZE = 0x8000;

USBBridge::USBBridge(Twili *twili) :
	twili(twili),
	ds(ResultCode::AssertOk(trn::service::usb::ds::DS::Initialize(2))),
	usb_state_change_event(ResultCode::AssertOk(ds.GetStateChangeEvent())),
	request_reader(this),
	request_meta_buffer(0x1000),
	response_meta_buffer(0x1000),
	request_data_buffer(TRANSFER_BUFFER_SIZE),
	response_data_buffer(TRANSFER_BUFFER_SIZE) {
	
	interface = ResultCode::AssertOk(
		ds.GetInterface(interface_descriptor, "twili_bridge"));
	endpoint_response_meta = ResultCode::AssertOk(
		interface->GetEndpoint(endpoint_in_descriptor)); // into host
	endpoint_request_meta = ResultCode::AssertOk(
		interface->GetEndpoint(endpoint_out_descriptor)); // out from host
	endpoint_response_data = ResultCode::AssertOk(
		interface->GetEndpoint(endpoint_in_descriptor)); // into host
	endpoint_request_data = ResultCode::AssertOk(
		interface->GetEndpoint(endpoint_out_descriptor)); // out from host
	ResultCode::AssertOk(interface->Enable());

	AddRequestHandler(CommandID::RUN, std::bind(&Twili::Run, twili, std::placeholders::_1, std::placeholders::_2));
	AddRequestHandler(CommandID::REBOOT, std::bind(&Twili::Reboot, twili, std::placeholders::_1, std::placeholders::_2));
	AddRequestHandler(CommandID::COREDUMP, std::bind(&Twili::CoreDump, twili, std::placeholders::_1, std::placeholders::_2));
	
	// wait for USB to come up
	while(USBStateChangeCallback()) {
		usb_state_change_event.ResetSignal();
		usb_state_change_event.WaitSignal(100000000);
	}

	request_reader.Begin();
}

usb_ds_report_entry_t *USBBridge::FindReport(std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint, usb_ds_report_t &report, uint32_t urb_id) {
	report = ResultCode::AssertOk(endpoint->GetReportData());
	usb_ds_report_entry_t *entry = nullptr;
	for(uint32_t i = 0; i < report.entry_count; i++) {
		if(report.entries[i].urb_id == urb_id) {
			return &report.entries[i];
		}
	}
	throw new ResultError(TWILI_ERR_FATAL_USB_TRANSFER);
}

trn::Result<std::nullopt_t> USBBridge::PostBufferSync(std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint, uint8_t *buffer, size_t size) {
	auto urb_id = endpoint->PostBufferAsync(buffer, size);
	if(!urb_id) {
		printf("[USBB] failed to post buffer async\n");
		return tl::make_unexpected(urb_id.error());
	}
	auto r = endpoint->completion_event.WaitSignal(30000000000);
	if(!r) {
		printf("[USBB] failed to wait on completion event\n");
		//return tl::make_unexpected(r.error());
	} else {
		r = endpoint->completion_event.ResetSignal();
		if(!r) {
			printf("[USBB] failed to reset signal\n");
			return tl::make_unexpected(r.error());
		}
	}
	
	usb_ds_report_t report;
	auto entry = FindReport(endpoint, report, *urb_id);
	if(entry->urb_status != 3) {
		printf("[USBB] URB status %d\n", entry->urb_status);
		throw new ResultError(TWILI_ERR_USB_TRANSFER);
	}
	if(entry->transferred_size < size) {
		printf("[USBB] didn't send all bytes, posting again...\n");
		return PostBufferSync(endpoint, buffer + entry->transferred_size, size - entry->transferred_size);
	}
	return std::nullopt;
}

bool USBBridge::USBStateChangeCallback() {
	//printf("USB state change signalled\n");
	if(ResultCode::AssertOk(ds.GetState()) == trn::service::usb::ds::State::INITIALIZED) {
		printf("finished USB bringup\n");
		return false;
	} else {
		return true;
	}
}

void USBBridge::AddRequestHandler(CommandID id, RequestHandler handler) {
	request_handlers[(uint32_t) id] = handler;
}

USBBridge::~USBBridge() {
}

USBBridge::USBRequestReader::USBRequestReader(twili::usb::USBBridge *bridge) : bridge(bridge) {
}

USBBridge::USBRequestReader::~USBRequestReader() {
}

void USBBridge::USBRequestReader::Begin() {
	meta_completion_wait =
		bridge->twili->event_waiter.Add(
			bridge->endpoint_request_meta->completion_event, [this]() {
				this->MetadataTransactionCompleted();
				this->PostMetaBuffer();
				return true;
			});

	data_completion_wait =
		bridge->twili->event_waiter.Add(
			bridge->endpoint_request_data->completion_event, [this]() {
				this->DataTransactionCompleted();
				return true;
			});

	PostMetaBuffer();
}

void USBBridge::USBRequestReader::PostMetaBuffer() {
	meta_urb_id = ResultCode::AssertOk(
		bridge->endpoint_request_meta->PostBufferAsync(
			bridge->request_meta_buffer.data, sizeof(TransactionHeader)));
}

void USBBridge::USBRequestReader::PostDataBuffer() {
	size_t size = bridge->request_data_buffer.size;
	if(current_header.payload_size - current_payload.size() < size) {
		size = current_header.payload_size - current_payload.size();
	}

	data_urb_id = ResultCode::AssertOk(
		bridge->endpoint_request_data->PostBufferAsync(
			bridge->request_data_buffer.data, size));
}

void USBBridge::USBRequestReader::MetadataTransactionCompleted() {
	usb_ds_report_t report;
	auto entry = USBBridge::FindReport(bridge->endpoint_request_meta, report, meta_urb_id);
	if(entry->urb_status != 3) {
		printf("Meta URB status (%d) != 3\n", entry->urb_status);
		return;
	}
	if(entry->transferred_size == 0) {
		return;
	}
	if(entry->transferred_size != sizeof(TransactionHeader)) {
		return;
	}
	memcpy(&current_header, bridge->request_meta_buffer.data, sizeof(TransactionHeader));
	printf("got header, command %u, payload size 0x%lx\n", current_header.command_id, current_header.payload_size);
	
	//bridge->endpoint_request_data->Stall(); // TODO
	current_payload.clear();
	if(current_header.payload_size > 0) {
		current_payload.reserve(current_header.payload_size);
		PostDataBuffer();
	} else {
		ProcessCommand();
	}
}

void USBBridge::USBRequestReader::DataTransactionCompleted() {
	usb_ds_report_t report;
	auto entry = USBBridge::FindReport(bridge->endpoint_request_data, report, data_urb_id);
	if(entry->urb_status != 3) {
		printf("Data URB status (%d) != 3\n", entry->urb_status);
		return;
	}
	if(current_payload.size() + entry->transferred_size > current_header.payload_size) {
		printf("Overshot payload size\n");
		USBResponseWriter r(bridge, current_header.tag);
		r.BeginError(TWILI_ERR_USB_TRANSFER, 0);
		return;
	}
	std::copy( // append data to current_payload
		bridge->request_data_buffer.data,
		bridge->request_data_buffer.data + entry->transferred_size,
		current_payload.insert(current_payload.end(), entry->transferred_size, 0));
	
	if(current_payload.size() < current_header.payload_size) {
		PostDataBuffer();
	} else {
		ProcessCommand();
	}
}

void USBBridge::USBRequestReader::ProcessCommand() {
	USBResponseWriter response(bridge, current_header.tag);
	auto i = bridge->request_handlers.find(current_header.command_id);
	if(i == bridge->request_handlers.end()) {
		response.BeginError(TWILI_ERR_BAD_REQUEST, 0);
		return;
	}
	trn::Result<std::nullopt_t> r = std::nullopt;
	try {
		r = (*i).second(current_payload, response);
	} catch(trn::ResultError *e) {
		r = tl::make_unexpected(e->code);
	}
	if(!response.HasBegun()) {
		if(r) {
			response.BeginOk(0);
		} else {
			response.BeginError(r.error(), 0);
		}
	}
}

USBBridge::USBResponseWriter::USBResponseWriter(USBBridge *bridge, uint32_t tag) :
	bridge(bridge),
	tag(tag) {
	
}

size_t USBBridge::USBResponseWriter::GetMaxTransferSize() {
	return bridge->response_data_buffer.size;
}

bool USBBridge::USBResponseWriter::HasBegun() {
	return has_begun;
}

trn::Result<std::nullopt_t> USBBridge::USBResponseWriter::BeginOk(size_t payload_size) {
	if(has_begun) {
		throw new ResultError(TWILI_ERR_FATAL_USB_TRANSFER);
	}
	TransactionHeader hdr;
	hdr.result_code = 0;
	hdr.tag = tag;
	hdr.payload_size = payload_size;
	has_begun = true;

	expected_size = payload_size;
	
	memcpy(bridge->response_meta_buffer.data, &hdr, sizeof(hdr));
	auto r = USBBridge::PostBufferSync(bridge->endpoint_response_meta, bridge->response_meta_buffer.data, sizeof(hdr));
	if(!r) {
		has_errored = true;
	}
	return r;
}

trn::Result<std::nullopt_t> USBBridge::USBResponseWriter::BeginError(ResultCode code, size_t payload_size) {
	if(has_begun) {
		throw new ResultError(TWILI_ERR_FATAL_USB_TRANSFER);
	}
	TransactionHeader hdr;
	hdr.result_code = code.code;
	hdr.tag = tag;
	hdr.payload_size = payload_size;
	has_begun = true;

	expected_size = payload_size;
	
	memcpy(bridge->response_meta_buffer.data, &hdr, sizeof(hdr));
	auto r = USBBridge::PostBufferSync(bridge->endpoint_response_meta, bridge->response_meta_buffer.data, sizeof(hdr));
	if(!r) {
		has_errored = true;
	}
	return r;
}

trn::Result<std::nullopt_t> USBBridge::USBResponseWriter::Write(uint8_t *data, size_t size) {
	auto max_size = bridge->response_data_buffer.size;
	while(size > max_size) {
		Write(data, max_size);
		data+= max_size;
		size-= max_size;
	}
	memcpy(bridge->response_data_buffer.data, data, size);
	auto r = USBBridge::PostBufferSync(bridge->endpoint_response_data, bridge->response_data_buffer.data, size);
	if(r) {
		transferred_size+= size;
	} else {
		has_errored = true;
	}
	return r;
}

USBBridge::USBResponseWriter::~USBResponseWriter() {
	if(!has_errored && transferred_size != expected_size) {
		throw new ResultError(TWILI_ERR_RESPONSE_SIZE_MISMATCH);
	}
}

USBBridge::USBBuffer::USBBuffer(size_t size) : size(size) {
	data = (uint8_t*) alloc_pages(size, size, nullptr);
	if(data == NULL) {
		throw new ResultError(LIBTRANSISTOR_ERR_OUT_OF_MEMORY);
	}
	auto r = ResultCode::ExpectOk(svcSetMemoryAttribute(data, size, 0x8, 0x8));
	if(!r) {
		free_pages(data);
		throw new ResultError(r.error());
	}
}

USBBridge::USBBuffer::~USBBuffer() {
	ResultCode::AssertOk(svcSetMemoryAttribute(data, size, 0x0, 0x0));
	free_pages(data);
}

}
}
