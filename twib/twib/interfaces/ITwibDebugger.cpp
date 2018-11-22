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

#include "ITwibDebugger.hpp"

#include<string.h>
#include "Protocol.hpp"

namespace twili {
namespace twib {

ITwibDebugger::ITwibDebugger(std::shared_ptr<RemoteObject> obj) : obj(obj) {
	
}

nx::DebugEvent ITwibDebugger::GetDebugEvent() {
	nx::DebugEvent event;
	obj->SendSmartSyncRequest(
		CommandID::GET_DEBUG_EVENT,
		out<nx::DebugEvent>(event));
	return event;
}

std::vector<uint64_t> ITwibDebugger::GetThreadContext(uint64_t thread_id) {
	struct ThreadContext {
		uint64_t regs[100];
	} tc;
	obj->SendSmartSyncRequest(
		CommandID::GET_THREAD_CONTEXT,
		in<uint64_t>(thread_id),
		out<ThreadContext>(tc));
	return std::vector<uint64_t>(&tc.regs[0], &tc.regs[100]);
}

} // namespace twib
} // namespace twili
