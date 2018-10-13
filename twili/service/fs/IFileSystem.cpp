#include "IFileSystem.hpp"

namespace twili {
namespace service {
namespace fs {

IFileSystem::IFileSystem(trn::ipc::server::IPCServer *server) : trn::ipc::server::Object(server) {
}

trn::ResultCode IFileSystem::Dispatch(trn::ipc::Message msg, uint32_t request_id) {
	printf("IFileSystem receives %d\n", request_id);
	switch(request_id) {
	case 8:
		return trn::ipc::server::RequestHandler<&IFileSystem::OpenFile>::Handle(this, msg);
	}
	return 1;
}

} // namespace fs
} // namespace service
} // namespace twili
