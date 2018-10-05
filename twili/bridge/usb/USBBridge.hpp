#pragma once

#include<libtransistor/cpp/waiter.hpp>
#include<libtransistor/cpp/ipc/usb_ds.hpp>

#include<vector>
#include<type_traits>
#include<map>
#include<functional>

#include "../../../common/Protocol.hpp"
#include "../ResponseOpener.hpp"

namespace twili {

class Twili;

namespace bridge {

class Object;

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

class USBBridge::ResponseState : public bridge::detail::ResponseState {
 public:
	ResponseState(USBBridge &bridge, uint32_t client_id, uint32_t tag);

	virtual size_t GetMaxTransferSize() override;
	virtual void SendHeader(protocol::MessageHeader &hdr) override;
	virtual void SendData(uint8_t *data, size_t size) override;
	virtual void Finalize() override;
	virtual uint32_t ReserveObjectId() override;
	virtual void InsertObject(std::pair<uint32_t, std::shared_ptr<Object>> &&pair) override;

 private:
	USBBridge &bridge;
};

} // namespace usb
} // namespace bridge
} // namespace twili
