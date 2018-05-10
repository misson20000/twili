#pragma once

#include<libtransistor/cpp/waiter.hpp>
#include<libtransistor/cpp/ipc/usb_ds.hpp>

#include<vector>
#include<type_traits>
#include<map>
#include<functional>

namespace twili {

class Twili;

namespace usb {

class USBBridge {
 public:
	struct TransactionHeader {
		union {
			uint32_t command_id; // request
			uint32_t result_code; // response
		};
		uint32_t tag;
		uint64_t payload_size;
	};
	class USBRequestReader {
	 public:
		USBRequestReader(USBBridge *bridge);
		~USBRequestReader();
		
		void Begin();
		void MetadataTransactionCompleted();
		void DataTransactionCompleted();
		void PostMetaBuffer();
		void PostDataBuffer();
		void ProcessCommand();
		
	 private:
		USBBridge *bridge;
		
		std::shared_ptr<trn::WaitHandle> meta_completion_wait;
		std::shared_ptr<trn::WaitHandle> data_completion_wait;
		
		uint32_t meta_urb_id;
		uint32_t data_urb_id;
		
		TransactionHeader current_header;
		std::vector<uint8_t> current_payload;
	};
	class USBBuffer {
	 public:
		USBBuffer(size_t size);
		~USBBuffer();
		uint8_t *data;
		size_t size;
	};
	class USBResponseWriter;

	using RequestHandler = std::function<trn::Result<std::nullopt_t>(std::vector<uint8_t>, USBResponseWriter&)>;
	
	enum class CommandID : uint32_t {
		RUN = 10,
		REBOOT = 11,
		COREDUMP = 12,
      TERMINATE = 13,
	};
	
	USBBridge(Twili *twili);
	~USBBridge();
	
	static usb_ds_report_entry_t *FindReport(std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint, usb_ds_report_t &buffer, uint32_t urb_id);
	static trn::Result<std::nullopt_t> PostBufferSync(std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint, uint8_t *buffer, size_t size);

	void AddRequestHandler(CommandID id, RequestHandler handler);
	
 private:
	Twili *twili;
	std::shared_ptr<trn::service::usb::ds::Interface> interface;
	std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint_response_meta;
	std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint_request_meta;
	std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint_response_data;
	std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint_request_data;
	std::map<uint64_t, RequestHandler> request_handlers;
	
	USBRequestReader request_reader;
	USBBuffer request_meta_buffer;
	USBBuffer response_meta_buffer;
	USBBuffer request_data_buffer;
	USBBuffer response_data_buffer;

	trn::service::usb::ds::DS ds;
	trn::KEvent usb_state_change_event;
	
	bool USBStateChangeCallback();
};

class USBBridge::USBResponseWriter {
 public:
	USBResponseWriter(USBBridge *bridge, uint32_t tag);
	~USBResponseWriter();
	
	size_t GetMaxTransferSize();
	bool HasBegun();
	trn::Result<std::nullopt_t> BeginOk(size_t payload_size);
	trn::Result<std::nullopt_t> BeginError(trn::ResultCode code, size_t payload_size);
	trn::Result<std::nullopt_t> Write(uint8_t *data, size_t size);
	
	template<typename T>
	trn::Result<std::nullopt_t> Write(std::vector<T> data) {
		static_assert(std::is_standard_layout<T>::value, "T must be standard layout");
		return Write((uint8_t*) data.data(), data.size() * sizeof(T));
	}
	
	template<typename T>
	trn::Result<std::nullopt_t> Write(T data) {
		static_assert(std::is_standard_layout<T>::value, "T must be standard layout");
		return Write((uint8_t*) &data, sizeof(data));
	}
 private:
	USBBridge *bridge;
	uint32_t tag;
	size_t transferred_size = 0;
	size_t expected_size;
	bool has_begun = false;
	bool has_errored = false;
};

}
}
