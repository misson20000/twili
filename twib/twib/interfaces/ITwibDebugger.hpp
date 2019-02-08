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

#include<vector>
#include<optional>
#include<tuple>

#include "../RemoteObject.hpp"
#include "../DebugTypes.hpp"

namespace twili {
namespace twib {

class ITwibDebugger {
 public:
	ITwibDebugger(std::shared_ptr<RemoteObject> obj);

	using CommandID = protocol::ITwibDebugger::Command;

	std::tuple<nx::MemoryInfo, nx::PageInfo> QueryMemory(uint64_t addr);
	std::vector<uint8_t> ReadMemory(uint64_t addr, uint64_t size);
	void WriteMemory(uint64_t addr, std::vector<uint8_t> &bytes);
	std::optional<nx::DebugEvent> GetDebugEvent();
	std::vector<uint64_t> GetThreadContext(uint64_t thread_id);
	void SetThreadContext(uint64_t thread_id, std::vector<uint64_t> registeres);
	void ContinueDebugEvent(uint32_t flags, std::vector<uint64_t> thread_ids);
	void BreakProcess();
	void AsyncWait(std::function<void(uint32_t)> &&cb);
 private:
	std::shared_ptr<RemoteObject> obj;
};

} // namespace twib
} // namespace twili
