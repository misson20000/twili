//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2021 misson20000 <xenotoad@xenotoad.net>
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

#pragma once

#include<algorithm>

#include<libtransistor/cpp/types.hpp>

#include "SystemVersion.hpp"
#include "hos_types.hpp"

namespace twili {
namespace tipc {

namespace detail {

struct RawLayoutItem {
	size_t size;
	size_t alignment;
};

template<RawLayoutItem... Items>
struct RawLayoutPack;

template<size_t offset, typename Pack>
struct RawLayout;

template<size_t offset, RawLayoutItem Item0, RawLayoutItem... Items>
struct RawLayout<offset, RawLayoutPack<Item0, Items...>> {	
	constexpr static size_t Offset = (offset + (Item0.alignment - 1)) / (Item0.alignment) * (Item0.alignment);
	constexpr static size_t Size = Item0.size;

	using Next = RawLayout<Offset + Size, RawLayoutPack<Items...>>;

	constexpr static size_t TotalSize = Next::TotalSize;
	constexpr static size_t TotalCount = (TotalSize + 3) / 4;

	template<typename Arg0, typename... Args>
	static void Pack(uint8_t *buffer, typename Arg0::Type &arg0, typename Args::Type &... args) {
		Arg0::FillRaw(arg0, buffer + Offset);
		Next::template Pack<Args...>(buffer, args...);
	}

	template<typename Arg0, typename... Args>
	static void Unpack(const uint8_t *buffer, typename Arg0::Type &arg0, typename Args::Type &... args) {
		Arg0::TakeRaw(arg0, buffer + Offset);
		Next::template Unpack<Args...>(buffer, args...);
	}
};

template<size_t offset>
struct RawLayout<offset, RawLayoutPack<>> {
	constexpr static size_t TotalSize = offset;
	constexpr static size_t TotalCount = 0;

	template<typename... Args>
	static void Pack(uint8_t *buffer) {
		// done
		static_assert(sizeof...(Args) == 0);
	}

	template<typename... Args>
	static void Unpack(const uint8_t *buffer) {
		// done
		static_assert(sizeof...(Args) == 0);
	}
};

template<typename T>
struct TipcInTypeHandlerBase {
	using Type = T;
	
	inline static constexpr RawLayoutItem GetRawLayoutItem() {
		return {0, 1};
	}

	inline static bool HasSpecialHeader() {
		return false;
	}

	inline static bool HasProcessId() {
		return false;
	}

	inline static uint32_t GetNumHandles() {
		return 0;
	}

	inline static void FillHandle(T &value, uint32_t *buffer, uint32_t &i) {
		// no-op
	}
	
	inline static void FillRaw(T &value, uint8_t *buffer) {
		// no-op
	}
};

template<typename T>
struct TipcInTypeHandler;

template<typename T>
struct TipcInTypeHandler<trn::ipc::InRaw<T>> : public TipcInTypeHandlerBase<trn::ipc::InRaw<T>> {
	inline static constexpr RawLayoutItem GetRawLayoutItem() {
		return { sizeof(T), alignof(T) };
	}

	inline static void FillRaw(trn::ipc::InRaw<T> &value, uint8_t *buffer) {
		std::copy_n((uint8_t*) &value.value, sizeof(T), buffer);
	}
};

template<typename T>
struct TipcInTypeHandler<trn::ipc::OutRaw<T>> : public TipcInTypeHandlerBase<trn::ipc::OutRaw<T>> {
};

template<>
struct TipcInTypeHandler<trn::ipc::InHandle<uint32_t, trn::ipc::copy>> : public TipcInTypeHandlerBase<trn::ipc::InHandle<uint32_t, trn::ipc::copy>> {
	inline static bool HasSpecialHeader() {
		return true;
	}

	inline static uint32_t GetNumHandles() {
		return 1;
	}

	inline static void FillHandle(trn::ipc::InHandle<uint32_t, trn::ipc::copy> &value, uint32_t *buffer, uint32_t &i) {
		buffer[i++] = value.handle;
	}
};

template<>
struct TipcInTypeHandler<trn::ipc::OutHandle<uint32_t, trn::ipc::copy>> : public TipcInTypeHandlerBase<trn::ipc::OutHandle<uint32_t, trn::ipc::copy>> {
};

template<typename T>
struct TipcInTypeHandler<trn::ipc::OutObject<T>> : public TipcInTypeHandlerBase<trn::ipc::OutObject<T>> {
};

template<>
struct TipcInTypeHandler<trn::ipc::InPid> : public TipcInTypeHandlerBase<trn::ipc::InPid> {
	inline static bool HasSpecialHeader() {
		return true;
	}

	inline static bool HasProcessId() {
		return true;
	}
};

template<>
struct TipcInTypeHandler<trn::ipc::OutPid> : public TipcInTypeHandlerBase<trn::ipc::OutPid> {
};

template<typename T>
struct TipcOutTypeHandlerBase {
	using Type = T;

	inline static constexpr RawLayoutItem GetRawLayoutItem() {
		return {0, 1};
	}

	inline static bool HasSpecialHeader() {
		return false;
	}

	inline static bool HasProcessId() {
		return false;
	}

	inline static uint32_t GetNumHandles() {
		return 0;
	}

	inline static uint32_t GetNumObjects() {
		return 0;
	}

	inline static void TakeHandle(T &value, const uint32_t *buffer, uint32_t &i) {
		// no-op
	}

	inline static void TakeObject(T &value, const uint32_t *buffer, uint32_t &i) {
		// no-op
	}

	inline static void TakeRaw(T &value, const uint8_t *buffer) {
		// no-op
	}

	inline static void TakePid(T &value, uint64_t pid) {
		// no-op
	}
};

template<typename T>
struct TipcOutTypeHandler;

template<typename T>
struct TipcOutTypeHandler<trn::ipc::InRaw<T>> : public TipcOutTypeHandlerBase<trn::ipc::InRaw<T>> {
};

template<typename T>
struct TipcOutTypeHandler<trn::ipc::OutRaw<T>> : public TipcOutTypeHandlerBase<trn::ipc::OutRaw<T>> {
	inline static constexpr RawLayoutItem GetRawLayoutItem() {
		return { sizeof(T), alignof(T) };
	}

	inline static void TakeRaw(trn::ipc::OutRaw<T> &value, const uint8_t *buffer) {
		std::copy_n(buffer, sizeof(T), (uint8_t*) value.value);
	}
};

template<>
struct TipcOutTypeHandler<trn::ipc::InHandle<uint32_t, trn::ipc::copy>> : public TipcOutTypeHandlerBase<trn::ipc::InHandle<uint32_t, trn::ipc::copy>> {
};

template<>
struct TipcOutTypeHandler<trn::ipc::OutHandle<uint32_t, trn::ipc::copy>> : public TipcOutTypeHandlerBase<trn::ipc::OutHandle<uint32_t, trn::ipc::copy>> {
	inline static bool HasSpecialHeader() {
		return true;
	}

	inline static uint32_t GetNumHandles() {
		return 1;
	}

	inline static void TakeHandle(trn::ipc::OutHandle<uint32_t, trn::ipc::copy> &value, const uint32_t *buffer, uint32_t &i) {
		*value.handle = buffer[i++];
	}
};

template<typename T>
struct TipcOutTypeHandler<trn::ipc::OutObject<T>> : public TipcOutTypeHandlerBase<trn::ipc::OutObject<T>> {
	inline static bool HasSpecialHeader() {
		return true;
	}

	inline static uint32_t GetNumObjects() {
		return 1;
	}

	inline static void TakeObject(trn::ipc::OutObject<T> &value, const uint32_t *buffer, uint32_t &i) {
		*value.value = T(buffer[i++]);
	}
};

template<>
struct TipcOutTypeHandler<trn::ipc::InPid> : public TipcOutTypeHandlerBase<trn::ipc::InPid> {
};

template<>
struct TipcOutTypeHandler<trn::ipc::OutPid> : public TipcOutTypeHandlerBase<trn::ipc::OutPid> {
	inline static bool HasSpecialHeader() {
		return true;
	}

	inline static bool HasProcessId() {
		return true;
	}

	inline static void TakePid(trn::ipc::OutPid &value, uint64_t pid) {
		*value.value = pid;
	}
};

} // namespace detail

template<uint32_t request_tag, typename... Args>
void PackRequest(uint32_t *buffer, Args &... args) {
	uint32_t i = 0;

	// MessageHeader
	
	buffer[i++] =
		((request_tag & 0xffff) << 0) | // Tag
		((0 & 0xf) << 16) | // PointerCount
		((0 & 0xf) << 20) | // SendCount
		((0 & 0xf) << 24) | // ReceiveCount
		((0 & 0xf) << 28); // ExchangeCount

	uint32_t raw_count;

	using Layout = detail::RawLayout<0, detail::RawLayoutPack<detail::TipcInTypeHandler<Args>::GetRawLayoutItem()...>>;
	
	buffer[i++] =
		((Layout::TotalCount & 0x3ff) << 0) | // RawCount
		((0 & 0xf) << 10) | // ReceiveListCount
		(((detail::TipcInTypeHandler<Args>::HasSpecialHeader() || ...) ? 1 : 0) << 31); // HasSpecialHeader

	if((detail::TipcInTypeHandler<Args>::HasSpecialHeader() || ...)) {
		// SpecialHeader

		buffer[i++] =
			(((detail::TipcInTypeHandler<Args>::HasProcessId() || ...) ? 1 : 0) << 0) | // HasProcessId
			(((detail::TipcInTypeHandler<Args>::GetNumHandles() + ... + 0) & 0xf) << 1) | // CopyHandleCount
			((0) << 5); // MoveHandleCount (tipc requests can't move handles?)

		if((detail::TipcInTypeHandler<Args>::HasProcessId() || ...)) {
			buffer[i++] = 0;
			buffer[i++] = 0;
		}

		(detail::TipcInTypeHandler<Args>::FillHandle(args, buffer, i), ...);
	}

	// Pointer descriptors

	// Send descriptors

	// Receive descriptors

	// Exchange descriptors

	Layout::template Pack<detail::TipcInTypeHandler<Args>...>((uint8_t*) (buffer + i), args...);
	i+= Layout::TotalCount;

	// Receive list
}

template<uint32_t expected_tag, typename... Args>
trn::ResultCode UnpackResponse(uint32_t *buffer, Args &... args) {
	uint32_t i = 0;

	// MessageHeader
	uint32_t mh0 = buffer[i++];
	uint32_t mh1 = buffer[i++];

	uint32_t tag = (mh0 >> 0) & 0xffff;
	// PointerCount
	// SendCount
	// ReceiveCount
	// ExchangeCount

	if(tag != expected_tag) {
		return TWILI_TIPC_ERR_UNEXPECTED_RESPONSE_TAG;
	}

	uint32_t raw_count = (mh1 >> 0) & 0x3ff; // RawCount
	// ReceiveListCount
	bool has_special_header = (mh1 >> 31) != 0; // HasSpecialHeader

	// needs at least 1 word for result
	if(raw_count < 1) {
		return TWILI_TIPC_ERR_UNEXPECTED_RESPONSE_RAW_COUNT;
	}

	using Layout = detail::RawLayout<0, detail::RawLayoutPack<detail::TipcOutTypeHandler<Args>::GetRawLayoutItem()...>>;
	
	// minus 1 because result
	if(raw_count - 1 != Layout::TotalCount) {
		return TWILI_TIPC_ERR_UNEXPECTED_RESPONSE_RAW_COUNT;
	}

	if(has_special_header != (detail::TipcOutTypeHandler<Args>::HasSpecialHeader() || ...)) {
		return TWILI_TIPC_ERR_UNEXPECTED_RESPONSE_SPECIAL_HEADER;
	}

	if(has_special_header) {
		// SpecialHeader
		uint32_t sh0 = buffer[i++];
		
		bool has_process_id = ((sh0 >> 0) & 0x1) != 0; // HasProcessId
		uint32_t copy_handle_count = (sh0 >> 1) & 0xf; // CopyHandleCount
		uint32_t move_handle_count = (sh0 >> 5) & 0xf; // MoveHandleCount

		if(has_process_id != (detail::TipcOutTypeHandler<Args>::HasProcessId() || ...)) {
			return TWILI_TIPC_ERR_UNEXPECTED_RESPONSE_PID;
		}
		
		if(copy_handle_count != (detail::TipcOutTypeHandler<Args>::GetNumHandles() + ...)) {
			return TWILI_TIPC_ERR_UNEXPECTED_RESPONSE_COPY_HANDLE_COUNT;
		}

		if(move_handle_count != (detail::TipcOutTypeHandler<Args>::GetNumObjects() + ...)) {
			return TWILI_TIPC_ERR_UNEXPECTED_RESPONSE_MOVE_HANDLE_COUNT;
		}

		if(has_process_id) {
			uint64_t pid = *(uint64_t *) (buffer + i);
			i+= 2;

			(detail::TipcOutTypeHandler<Args>::TakePid(args, pid), ...);
		}

		(detail::TipcOutTypeHandler<Args>::TakeHandle(args, buffer, i), ...);
		(detail::TipcOutTypeHandler<Args>::TakeObject(args, buffer, i), ...);
	}

	uint32_t result = buffer[i++];
	if(result != RESULT_OK) {
		return result;
	}

	Layout::template Unpack<detail::TipcOutTypeHandler<Args>...>((uint8_t*) (buffer + i), args...);
	i+= Layout::TotalCount;

	return RESULT_OK;
}

class Object {
 public:
	Object() : is_valid(false) {
	}
	
	Object(handle_t handle) : is_valid(true), handle(handle) {
	}

	Object(Object &&other) : is_valid(other.is_valid), handle(other.handle) {
		other.is_valid = false;
	}

	Object(Object &other) = delete;

	Object &operator=(Object &&other) {
		if(is_valid) {
			Close();
		}

		this->is_valid = other.is_valid;
		this->handle = other.handle;

		other.is_valid = false;

		return *this;
	}
	
	Object &operator=(Object &other) = delete;

	inline ~Object() {
		if(is_valid) {
			Close();
		}
	}

	inline void Close() {
		uint32_t *buffer = get_tls()->ipc_buffer;
		
		PackRequest<15>(buffer);
		svcSendSyncRequest(handle);
		svcCloseHandle(handle);

		is_valid = false;
	}

	template<uint32_t id, typename... Args>
	inline trn::ResultCode SendSyncRequest(Args &&... args) {
		result_t r;
		
		uint32_t *buffer = get_tls()->ipc_buffer;
		
		PackRequest<id + 16, Args...>(buffer, args...);

		r = svcSendSyncRequest(handle);
		if(r != RESULT_OK) {
			return r;
		}

		return UnpackResponse<id + 16, Args...>(buffer, args...);
	}
 private:
	bool is_valid = false;
	handle_t handle;
};

} // namespace tipc
}
