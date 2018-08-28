#pragma once

#include<libtransistor/cpp/waiter.hpp>
#include<libtransistor/cpp/ipc/usb_ds.hpp>

#include<vector>
#include<type_traits>
#include<map>
#include<functional>

#include "twib/Protocol.hpp"

namespace twili {

class Twili;

namespace bridge {

class Object;

} // namespace bridge

namespace usb {

class USBBuffer {
 public:
	USBBuffer(size_t size);
	~USBBuffer();
	uint8_t *data;
	size_t size;
};

class USBBridge {
 public:
	class RequestReader {
	 public:
		RequestReader(USBBridge *bridge);
		~RequestReader();
		
		void Begin();
		void MetadataTransactionCompleted();
		void DataTransactionCompleted();
		void PostMetaBuffer();
		void PostDataBuffer();
		void PostObjectBuffer();
		void ProcessCommand();
		
	 private:
		USBBridge *bridge;
		
		std::shared_ptr<trn::WaitHandle> meta_completion_wait;
		std::shared_ptr<trn::WaitHandle> data_completion_wait;
		
		uint32_t meta_urb_id;
		uint32_t data_urb_id;
		uint32_t object_urb_id;
		
		protocol::MessageHeader current_header;
		std::vector<uint8_t> current_payload;
		std::vector<uint32_t> object_ids;
	};

	class ResponseState;
	class ResponseOpener;
	class ResponseWriter;
	using RequestHandler = std::function<trn::Result<std::nullopt_t>(std::vector<uint8_t>, ResponseOpener)>;
	
	USBBridge(Twili *twili, std::shared_ptr<bridge::Object> object_zero);
	~USBBridge();

	USBBridge(const USBBridge&) = delete;
	USBBridge &operator=(USBBridge const&) = delete;
	
	static usb_ds_report_entry_t *FindReport(std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint, usb_ds_report_t &buffer, uint32_t urb_id);
	static void PostBufferSync(std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint, uint8_t *buffer, size_t size);

	void ResetInterface();
	
 private:
	Twili *twili;
	std::shared_ptr<trn::service::usb::ds::Interface> interface;
	std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint_response_meta;
	std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint_request_meta;
	std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint_response_data;
	std::shared_ptr<trn::service::usb::ds::Endpoint> endpoint_request_data;
	std::shared_ptr<trn::WaitHandle> state_change_wait;

	uint32_t object_id = 1;
	std::map<uint32_t, std::shared_ptr<bridge::Object>> objects;
	
	RequestReader request_reader;
	USBBuffer request_meta_buffer;
	USBBuffer response_meta_buffer;
	USBBuffer request_data_buffer;
	USBBuffer response_data_buffer;

	trn::service::usb::ds::DS ds;
	trn::KEvent usb_state_change_event;
	
	bool USBStateChangeCallback();
};

class USBBridge::ResponseState {
 public:
	ResponseState(USBBridge &bridge, uint32_t client_id, uint32_t tag);
	void Finalize();

	USBBridge &bridge;
	uint32_t client_id;
	uint32_t tag;

	std::vector<std::shared_ptr<bridge::Object>> objects;
	size_t transferred_size = 0;
	size_t total_size = 0;
	uint32_t object_count;
	bool has_begun = false;
};

class USBBridge::ResponseOpener {
 public:
	ResponseOpener(std::shared_ptr<ResponseState> state);

	USBBridge::ResponseWriter BeginOk(size_t payload_size=0, uint32_t object_count=0);
	USBBridge::ResponseWriter BeginError(trn::ResultCode code, size_t payload_size=0, uint32_t object_count=0);
	
	template<typename T, typename... Args>
	std::shared_ptr<bridge::Object> MakeObject(Args... args) {
		uint32_t object_id = state->bridge.object_id++;
		std::shared_ptr<bridge::Object> obj = std::make_shared<T>(object_id, args...);
		state->bridge.objects.insert(std::pair<uint32_t, std::shared_ptr<bridge::Object>>(object_id, obj));
		return obj;
	}
	
 private:
	std::shared_ptr<ResponseState> state;
};

class USBBridge::ResponseWriter {
 public:
	ResponseWriter(std::shared_ptr<ResponseState> state);
	
	size_t GetMaxTransferSize();
	void Write(uint8_t *data, size_t size);
	void Write(std::string str);
	
	template<typename T>
	void Write(std::vector<T> data) {
		static_assert(std::is_standard_layout<T>::value, "T must be standard layout");
		return Write((uint8_t*) data.data(), data.size() * sizeof(T));
	}
	
	template<typename T>
	void Write(T data) {
		static_assert(std::is_standard_layout<T>::value, "T must be standard layout");
		return Write((uint8_t*) &data, sizeof(data));
	}

	uint32_t Object(std::shared_ptr<bridge::Object> object);

	void Finalize();
 private:
	std::shared_ptr<ResponseState> state;
};

}
}
