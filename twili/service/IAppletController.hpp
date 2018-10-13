#pragma once

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>

#include "../process/AppletProcess.hpp"

namespace twili {
namespace service {

class IAppletController : public trn::ipc::server::Object {
 public:
	IAppletController(trn::ipc::server::IPCServer *server, std::shared_ptr<process::AppletProcess> process);
	virtual ~IAppletController() override;
	
	virtual trn::ResultCode Dispatch(trn::ipc::Message msg, uint32_t request_id) override;

	trn::ResultCode SetResult(trn::ipc::InRaw<uint32_t> result);
 private:
	std::shared_ptr<process::AppletProcess> process;
};

} // namespace service
} // namespace twili
