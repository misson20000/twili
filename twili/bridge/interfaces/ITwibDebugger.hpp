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
