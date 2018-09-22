#pragma once

#include<libtransistor/cpp/types.hpp>

#include<memory>
#include<type_traits>
#include<vector>
#include<map>

#include "twib/Protocol.hpp"

namespace twili {
namespace bridge {

class Object;

namespace detail {

class ResponseState {
 public:
	inline ResponseState(uint32_t client_id, uint32_t tag) : client_id(client_id), tag(tag) {}
	virtual size_t GetMaxTransferSize() = 0;
	virtual void SendHeader(protocol::MessageHeader &hdr) = 0;
	virtual void SendData(uint8_t *data, size_t size) = 0;
	virtual void Finalize() = 0;
	virtual uint32_t ReserveObjectId() = 0;
	virtual void InsertObject(std::pair<uint32_t, std::shared_ptr<Object>> &&pair) = 0;

	const uint32_t client_id;
	const uint32_t tag;
	
	std::vector<std::shared_ptr<bridge::Object>> objects;
	size_t transferred_size = 0;
	size_t total_size = 0;
	uint32_t object_count = 0;
	bool has_begun = false;
};

} // namespace detail

class ResponseWriter {
 public:
	ResponseWriter(std::shared_ptr<detail::ResponseState> state);
	
	inline size_t GetMaxTransferSize() { return state->GetMaxTransferSize(); }
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
	std::shared_ptr<detail::ResponseState> state;
};

class ResponseOpener {
 public:
	ResponseOpener(std::shared_ptr<detail::ResponseState> state);
	ResponseWriter BeginOk(size_t payload_size=0, uint32_t object_count=0) const;
	ResponseWriter BeginError(trn::ResultCode code, size_t payload_size=0, uint32_t object_count=0) const;
	
	template<typename T, typename... Args>
	std::shared_ptr<Object> MakeObject(Args &&... args) const {
		uint32_t object_id = state->ReserveObjectId();
		std::shared_ptr<Object> obj = std::make_shared<T>(object_id, (std::forward<Args>(args))...);
		state->InsertObject(std::pair<uint32_t, std::shared_ptr<Object>>(object_id, obj));
		return obj;
	}
	
 private:
	std::shared_ptr<detail::ResponseState> state;
};

} // namespace bridge
} // namespace twili
