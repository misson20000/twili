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
namespace tool {

struct ThreadContext {
     uint64_t x[31];
     uint64_t sp, pc;
     uint32_t psr;
     uint32_t _pad;
     uint64_t fpr[32][2];
     uint32_t fpcr, fpsr;
     uint64_t tpidr;
};
static_assert(sizeof(ThreadContext) == 800, "sizeof(ThreadContext)");

class ITwibDebugger {
 public:
	ITwibDebugger(std::shared_ptr<RemoteObject> obj);

	using CommandID = protocol::ITwibDebugger::Command;

	std::tuple<nx::MemoryInfo, nx::PageInfo> QueryMemory(uint64_t addr);
	std::vector<uint8_t> ReadMemory(uint64_t addr, uint64_t size);
	void WriteMemory(uint64_t addr, std::vector<uint8_t> &bytes);
	std::optional<nx::DebugEvent> GetDebugEvent();
	ThreadContext GetThreadContext(uint64_t thread_id);
	void SetThreadContext(uint64_t thread_id, ThreadContext tc);

	// returns true if there are debug events pending
	bool ContinueDebugEvent(uint32_t flags, std::vector<uint64_t> thread_ids);
	
	void BreakProcess();
	void AsyncWait(std::function<void(uint32_t)> &&cb);
	uint64_t GetTargetEntry();
	void LaunchDebugProcess();
	std::vector<nx::LoadedModuleInfo> GetNsoInfos();
	std::vector<nx::LoadedModuleInfo> GetNroInfos();
 private:
	std::shared_ptr<RemoteObject> obj;
};

} // namespace tool
} // namespace twib
} // namespace twili
