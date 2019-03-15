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

#include "ITwibProcessMonitor.hpp"

#include "Protocol.hpp"
#include "common/Logger.hpp"
#include "common/ResultError.hpp"
#include "err.hpp"

namespace twili {
namespace twib {
namespace tool {

ITwibProcessMonitor::ITwibProcessMonitor(std::shared_ptr<RemoteObject> obj) : obj(obj) {
}

uint64_t ITwibProcessMonitor::Launch() {
	uint64_t pid;
	obj->SendSmartSyncRequest(
		CommandID::LAUNCH,
		out(pid));
	return pid;
}

void ITwibProcessMonitor::AppendCode(std::vector<uint8_t> code) {
	obj->SendSmartSyncRequest(
		CommandID::APPEND_CODE,
		in(code));
}

ITwibPipeWriter ITwibProcessMonitor::OpenStdin() {
	std::optional<ITwibPipeWriter> writer;
	obj->SendSmartSyncRequest(
		CommandID::OPEN_STDIN,
		out_object(writer));
	return *writer;
}

ITwibPipeReader ITwibProcessMonitor::OpenStdout() {
	std::optional<ITwibPipeReader> reader;
	obj->SendSmartSyncRequest(
		CommandID::OPEN_STDOUT,
		out_object(reader));
	return *reader;
}

ITwibPipeReader ITwibProcessMonitor::OpenStderr() {
	std::optional<ITwibPipeReader> reader;
	obj->SendSmartSyncRequest(
		CommandID::OPEN_STDERR,
		out_object(reader));
	return *reader;
}

uint32_t ITwibProcessMonitor::WaitStateChange() {
	uint32_t state;
	obj->SendSmartSyncRequest(
		CommandID::WAIT_STATE_CHANGE,
		out(state));
	return state;
}

} // namespace tool
} // namespace twib
} // namespace twili
