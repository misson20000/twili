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

#include "BridgeObject.hpp"

#include "Twibd.hpp"
#include "Messages.hpp"

namespace twili {
namespace twib {
namespace daemon {

BridgeObject::BridgeObject(Twibd &twibd, uint32_t device_id, uint32_t object_id) :
	twibd(twibd),
	device_id(device_id),
	object_id(object_id) {
}

BridgeObject::~BridgeObject() {
	// try to close object if valid
	if(valid) {
		LogMessage(Debug, "cleaning up lost object 0x%x", object_id);
		twibd.local_client->SendRequest(
			Request(
				nullptr, // local client overwrites this
				device_id,
				object_id,
				0xffffffff,
				0 // local client generates this
				)); // we don't care about the response
	}
}

} // namespace daemon
} // namespace twib
} // namespace twili
