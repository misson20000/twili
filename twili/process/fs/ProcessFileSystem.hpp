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
