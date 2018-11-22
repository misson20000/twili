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
#include "Logger.hpp"
#include "ResultError.hpp"
#include "err.hpp"

namespace twili {
namespace twib {

ITwibProcessMonitor::ITwibProcessMonitor(std::shared_ptr<RemoteObject> obj) : obj(obj) {
}

uint64_t ITwibProcessMonitor::Launch() {
	Response rs = obj->SendSyncRequest(protocol::ITwibProcessMonitor::Command::LAUNCH);
	if(rs.payload.size() < sizeof(uint64_t)) {
		throw ResultError(TWILI_ERR_BAD_RESPONSE);
	}
	return *(uint64_t*) rs.payload.data();
}

void ITwibProcessMonitor::AppendCode(std::vector<uint8_t> code) {
	obj->SendSyncRequest(protocol::ITwibProcessMonitor::Command::APPEND_CODE, code);
}

ITwibPipeWriter ITwibProcessMonitor::OpenStdin() {
	return ITwibPipeWriter(obj->SendSyncRequest(protocol::ITwibProcessMonitor::Command::OPEN_STDIN).objects[0]);
}

ITwibPipeReader ITwibProcessMonitor::OpenStdout() {
	return ITwibPipeReader(obj->SendSyncRequest(protocol::ITwibProcessMonitor::Command::OPEN_STDOUT).objects[0]);
}

ITwibPipeReader ITwibProcessMonitor::OpenStderr() {
	return ITwibPipeReader(obj->SendSyncRequest(protocol::ITwibProcessMonitor::Command::OPEN_STDERR).objects[0]);
}

uint32_t ITwibProcessMonitor::WaitStateChange() {
	Response rs = obj->SendSyncRequest(protocol::ITwibProcessMonitor::Command::WAIT_STATE_CHANGE);
	if(rs.payload.size() < sizeof(uint32_t)) {
		throw ResultError(TWILI_ERR_BAD_RESPONSE);
	}
	return *(uint32_t*) rs.payload.data();
}

} // namespace twib
} // namespace twili
