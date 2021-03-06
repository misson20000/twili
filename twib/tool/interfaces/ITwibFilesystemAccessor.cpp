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

#include "ITwibFilesystemAccessor.hpp"

#include "Protocol.hpp"

#include<cstring>

namespace twili {
namespace twib {
namespace tool {

ITwibFilesystemAccessor::ITwibFilesystemAccessor(std::shared_ptr<RemoteObject> obj) : obj(obj) {
	
}

bool ITwibFilesystemAccessor::CreateFile(uint32_t mode, size_t size, std::string path) {
	uint32_t r = obj->SendSmartSyncRequestWithoutAssert(
		CommandID::CREATE_FILE,
		in<uint32_t>(mode),
		in<uint64_t>(size),
		in<std::string>(path));
	if(r == 0x402) {
		// already exists
		return false;
	}
	if(r != 0) {
		throw ResultError(r);
	}
	return true;
}

void ITwibFilesystemAccessor::DeleteFile(std::string path) {
	obj->SendSmartSyncRequest(
		CommandID::DELETE_FILE,
		in<std::string>(path));
}

bool ITwibFilesystemAccessor::CreateDirectory(std::string path) {
	uint32_t r = obj->SendSmartSyncRequestWithoutAssert(
		CommandID::CREATE_DIRECTORY,
		in<std::string>(path));
	if(r == 0x402) {
		// already exists
		return false;
	}
	if(r != 0) {
		throw ResultError(r);
	}
	return true;
}

void ITwibFilesystemAccessor::DeleteDirectory(std::string path) {
	obj->SendSmartSyncRequest(
		CommandID::DELETE_DIRECTORY,
		in<std::string>(path));
}

void ITwibFilesystemAccessor::DeleteDirectoryRecursively(std::string path) {
	obj->SendSmartSyncRequest(
		CommandID::DELETE_DIRECTORY_RECURSIVELY,
		in<std::string>(path));
}

void ITwibFilesystemAccessor::RenameFile(std::string src, std::string dst) {
	obj->SendSmartSyncRequest(
		CommandID::RENAME_FILE,
		in<std::string>(src),
		in<std::string>(dst));
}

void ITwibFilesystemAccessor::RenameDirectory(std::string src, std::string dst) {
	obj->SendSmartSyncRequest(
		CommandID::RENAME_DIRECTORY,
		in<std::string>(src),
		in<std::string>(dst));
}

std::optional<bool> ITwibFilesystemAccessor::IsFile(std::string path) {
	uint32_t entry_type;
	uint32_t r = obj->SendSmartSyncRequestWithoutAssert(
		CommandID::GET_ENTRY_TYPE,
		in<std::string>(path),
		out<uint32_t>(entry_type));
	if(r == 0x202) {
		// not found
		return std::nullopt;
	}
	if(r != 0) {
		throw ResultError(r);
	}
	return entry_type > 0;
}

ITwibFileAccessor ITwibFilesystemAccessor::OpenFile(uint32_t mode, std::string path) {
	std::optional<ITwibFileAccessor> ifa;
	obj->SendSmartSyncRequest(
		CommandID::OPEN_FILE,
		in<uint32_t>(mode),
		in<std::string>(path),
		out_object<ITwibFileAccessor>(ifa));
	return *ifa;
}

ITwibDirectoryAccessor ITwibFilesystemAccessor::OpenDirectory(std::string path) {
	std::optional<ITwibDirectoryAccessor> ida;
	obj->SendSmartSyncRequest(
		CommandID::OPEN_DIRECTORY,
		in<std::string>(path),
		out_object<ITwibDirectoryAccessor>(ida));
	return *ida;
}

} // namespace tool
} // namespace twib
} // namespace twili
