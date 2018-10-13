#include "IAppletController.hpp"

#include "../twili.hpp"

#include "err.hpp"

namespace twili {
namespace service {

IAppletController::IAppletController(trn::ipc::server::IPCServer *server, std::shared_ptr<process::AppletProcess> process) : trn::ipc::server::Object(server), process(process) {
}

IAppletController::~IAppletController() {
	printf("destroying IAppletController\n");
	printf("marking process as exited\n");
	process->ChangeState(process::MonitoredProcess::State::Exited);
}

trn::ResultCode IAppletController::Dispatch(trn::ipc::Message msg, uint32_t request_id) {
	switch(request_id) {
	case 0:
		return trn::ipc::server::RequestHandler<&IAppletController::SetResult>::Handle(this, msg);
	}
	return 1;
}

trn::ResultCode IAppletController::SetResult(trn::ipc::InRaw<uint32_t> result) {
	process->SetResult(result.value);
	return RESULT_OK;
}

} // namespace service
} // namespace twili
