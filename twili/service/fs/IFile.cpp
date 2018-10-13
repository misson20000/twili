#include "IFile.hpp"

namespace twili {
namespace service {
namespace fs {

IFile::IFile(trn::ipc::server::IPCServer *server) : trn::ipc::server::Object(server) {
}

trn::ResultCode IFile::Dispatch(trn::ipc::Message msg, uint32_t request_id) {
	printf("IFile receives %d\n", request_id);
	switch(request_id) {
	case 0:
		return trn::ipc::server::RequestHandler<&IFile::Read>::Handle(this, msg);
	case 4:
		return trn::ipc::server::RequestHandler<&IFile::GetSize>::Handle(this, msg);
	}
	return 1;
}

} // namespace fs
} // namespace service
} // namespace twili
