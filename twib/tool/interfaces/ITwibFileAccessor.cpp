//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2019 misson20000 <xenotoad@xenotoad.net>
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

#include "ITwibFileAccessor.hpp"

#include "Protocol.hpp"

#include<cstring>

namespace twili {
namespace twib {
namespace tool {

ITwibFileAccessor::ITwibFileAccessor(std::shared_ptr<RemoteObject> obj) : obj(obj) {
	
}

std::vector<uint8_t> ITwibFileAccessor::Read(uint64_t offset, uint64_t size) {
	std::vector<uint8_t> vec;
	obj->SendSmartSyncRequest(
		CommandID::READ,
		in<uint64_t>(offset),
		in<uint64_t>(size),
		out<std::vector<uint8_t>>(vec));
	return vec;
}

void ITwibFileAccessor::Write(uint64_t offset, std::vector<uint8_t> &vec) {
	obj->SendSmartSyncRequest(
		CommandID::WRITE,
		in<uint64_t>(offset),
		in<std::vector<uint8_t>>(vec));
}

void ITwibFileAccessor::Flush() {
	obj->SendSmartSyncRequest(CommandID::FLUSH);
}

void ITwibFileAccessor::SetSize(size_t size) {
	obj->SendSmartSyncRequest(CommandID::SET_SIZE, in<size_t>(size));
}

size_t ITwibFileAccessor::GetSize() {
	size_t size;
	obj->SendSmartSyncRequest(CommandID::GET_SIZE, out<size_t>(size));
	return size;
}


} // namespace tool
} // namespace twib
} // namespace twili
