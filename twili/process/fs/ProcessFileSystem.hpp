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

#include "../../service/fs/IFileSystem.hpp"
#include "../../service/fs/IFile.hpp"

namespace twili {

class Twili;

namespace process {
namespace fs {

class ProcessFile;

class ProcessFileSystem {
 public:
	class IFileSystem : public service::fs::IFileSystem {
	 public:
		IFileSystem(trn::ipc::server::IPCServer *server, ProcessFileSystem &pfs);
		
		virtual trn::ResultCode OpenFile(trn::ipc::InRaw<uint32_t> mode, trn::ipc::Buffer<char, 0x19, 0> path, trn::ipc::OutObject<service::fs::IFile> &out) override;
	 private:
		ProcessFileSystem &pfs;
	};
		
	void SetRtld(std::shared_ptr<ProcessFile> file);
	void SetMain(std::shared_ptr<ProcessFile> file);
	void SetNpdm(std::shared_ptr<ProcessFile> file);
 private:
	std::shared_ptr<ProcessFile> rtld;
	std::shared_ptr<ProcessFile> main;
	std::shared_ptr<ProcessFile> npdm;

	class IFile : public service::fs::IFile {
	 public:
		IFile(trn::ipc::server::IPCServer *server, std::shared_ptr<ProcessFile> file);
		virtual ~IFile() override = default;

		virtual trn::ResultCode Read(trn::ipc::InRaw<uint32_t> unk0, trn::ipc::InRaw<uint64_t> offset, trn::ipc::InRaw<uint64_t> in_size, trn::ipc::OutRaw<uint64_t> out_size, trn::ipc::Buffer<uint8_t, 0x46, 0> out_buffer) override;
		virtual trn::ResultCode GetSize(trn::ipc::OutRaw<uint64_t> file_size) override;
	 private:
		std::shared_ptr<ProcessFile> file;
	};
};

} // namespace fs
} // namespace process
} // namespace twili
