#include "twili.hpp"
#include "IFile.hpp"

namespace twili {

IFile::IFile(trn::ipc::server::IPCServer *server, std::shared_ptr<std::vector<uint8_t>> file_data) :
	trn::ipc::server::Object(server), file_data(file_data) {
	printf("Created IFile\n");
}

trn::ResultCode IFile::Dispatch(trn::ipc::Message msg, uint32_t request_id) {
	switch(request_id) {
	case 0:
		return trn::ipc::server::RequestHandler<&IFile::Read>::Handle(this, msg);
	case 4:
		return trn::ipc::server::RequestHandler<&IFile::GetSize>::Handle(this, msg);
	}
	return 1;
}

trn::ResultCode IFile::Read(trn::ipc::InRaw<uint32_t> unk1, trn::ipc::InRaw<uint64_t> offset, trn::ipc::InRaw<uint64_t> size, trn::ipc::OutRaw<uint64_t> out_size, trn::ipc::Buffer<uint8_t, 0x46, 0> out_buf) {
	if (file_data->size() < offset.value) {
		return 1;
	}

	size_t realsize = std::min({ out_buf.size, size.value, file_data->size() - offset.value });
	std::copy_n(file_data->begin(), realsize, out_buf.data);
	out_size = realsize;

	return RESULT_OK;
}

trn::ResultCode IFile::GetSize(trn::ipc::OutRaw<uint64_t> size) {
	size = file_data->size();
	return RESULT_OK;
}

}
