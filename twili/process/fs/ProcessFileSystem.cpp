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

#include "ProcessFileSystem.hpp"

#include "ProcessFile.hpp"

#include "../../twili.hpp"
#include "../../service/fs/IFile.hpp"

namespace twili {
namespace process {
namespace fs {

ProcessFileSystem::IFileSystem::IFileSystem(trn::ipc::server::IPCServer *server, ProcessFileSystem &pfs) :
	service::fs::IFileSystem(server), pfs(pfs) {
}

void ProcessFileSystem::SetRtld(std::shared_ptr<ProcessFile> file) {
	rtld = file;
}
void ProcessFileSystem::SetMain(std::shared_ptr<ProcessFile> file) {
	main = file;
}
void ProcessFileSystem::SetNpdm(std::shared_ptr<ProcessFile> file) {
	npdm = file;
}

trn::ResultCode ProcessFileSystem::IFileSystem::OpenFile(trn::ipc::InRaw<uint32_t> mode, trn::ipc::Buffer<char, 0x19, 0> path, trn::ipc::OutObject<service::fs::IFile> &out) {
	std::shared_ptr<ProcessFile> file;
	if(!strncmp(path.data, "/main", path.size)) {
		file = pfs.main;
	} else if(!strncmp(path.data, "/rtld", path.size)) {
		file = pfs.rtld;
	} else if(!strncmp(path.data, "/main.npdm", path.size)) {
		file = pfs.npdm;
	}
	if(!file) {
		return 0x202; // file or directory does not exist
	}
	auto r = server->CreateObject<IFile>(this, file);
	if(r) {
		out.value = r.value();
		return RESULT_OK;
	} else {
		return r.error();
	}
}

ProcessFileSystem::IFile::IFile(trn::ipc::server::IPCServer *server, std::shared_ptr<ProcessFile> file) :
	service::fs::IFile(server), file(file) {
}

trn::ResultCode ProcessFileSystem::IFile::Read(trn::ipc::InRaw<uint32_t> unk0, trn::ipc::InRaw<uint64_t> offset, trn::ipc::InRaw<uint64_t> in_size, trn::ipc::OutRaw<uint64_t> out_size, trn::ipc::Buffer<uint8_t, 0x46, 0> out_buffer) {
	out_size = file->Read(offset.value, std::min(in_size.value, out_buffer.size), out_buffer.data);
	return 0;
}

trn::ResultCode ProcessFileSystem::IFile::GetSize(trn::ipc::OutRaw<uint64_t> file_size) {
	file_size = file->GetSize();
	return 0;
}

} // namespace fs
} // namespace process
} // namespace twili
