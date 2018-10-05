#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/waiter.hpp>

#include "../Object.hpp"
#include "../ResponseOpener.hpp"

namespace twili {

class Twili;

namespace bridge {

class ITwibDebugger : public bridge::Object {
 public:
	ITwibDebugger(uint32_t object_id, Twili &twili, trn::KDebug &&debug);
	virtual void HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	
 private:
	Twili &twili;
	trn::KDebug debug;
	std::shared_ptr<trn::WaitHandle> wait_handle;
	
	void QueryMemory(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void ReadMemory(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void WriteMemory(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void ListThreads(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void GetDebugEvent(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void GetThreadContext(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void BreakProcess(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void ContinueDebugEvent(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void SetThreadContext(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void GetNsoInfos(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
	void WaitEvent(std::vector<uint8_t> payload, bridge::ResponseOpener opener);
};

} // namespace bridge
} // namespace twili
