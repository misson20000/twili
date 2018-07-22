typedef bool _Bool;
#include<iostream>

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>
#include<libtransistor/cpp/ipcclient.hpp>
#include<libtransistor/thread.h>
#include<libtransistor/ipc/fatal.h>
#include<libtransistor/ipc/sm.h>
#include<libtransistor/ipc.h>
#include<libtransistor/ipc_helpers.h>
#include<libtransistor/loader_config.h>
#include<libtransistor/runtime_config.h>
#include<libtransistor/usb_serial.h>

#include<unistd.h>
#include<stdio.h>

#include "util.hpp"
#include "twili.hpp"
#include "process_creation.hpp"
#include "Process.hpp"
#include "ITwiliService.hpp"
#include "ITwibDeviceInterface.hpp"
#include "USBBridge.hpp"
#include "err.hpp"

using ResultCode = trn::ResultCode;
template<typename T>
using Result = trn::Result<T>;

void server_thread(void *arg) {
	twili::Twili *twili = (twili::Twili*) arg;
	while(!twili->destroy_flag) {
		ResultCode::AssertOk(twili->event_waiter.Wait(3000000000));
		twili->monitored_processes.remove_if([](const auto &proc) {
				return proc.destroy_flag;
			});
	}
	printf("twili terminating...\n");
	printf("terminating monitored processes...\n");
	for(twili::MonitoredProcess &proc : twili->monitored_processes) {
		proc.Terminate();
	}
	printf("twili server thread terminating\n");
}

int main() {
	uint64_t syscall_hints[2] = {0xffffffffffffffff, 0xffffffffffffffff};
	memcpy(loader_config.syscall_hints, syscall_hints, sizeof(syscall_hints));
	
	try {
		ResultCode::AssertOk(usb_serial_init());

		// set up serial console
		int usb_fd = usb_serial_open_fd();
		if(usb_fd < 0) {
			throw trn::ResultError(-usb_fd);
		}
		dup2(usb_fd, STDOUT_FILENO);
		dup2(usb_fd, STDERR_FILENO);
		dup2(usb_fd, STDIN_FILENO);
		dbg_set_file(fd_file_get(usb_fd));
		printf("brought up USB serial\n");

		// initialize twili
		twili::Twili twili;
		
		trn_thread_t thread;
		ResultCode::AssertOk(trn_thread_create(&thread, server_thread, &twili, 58, -2, 1024 * 64, NULL));
		ResultCode::AssertOk(trn_thread_start(&thread));
		
		ResultCode::AssertOk(sm_init());
		ipc_object_t sess;
		ResultCode::AssertOk(sm_get_service(&sess, "twili"));

		ResultCode::AssertOk(trn_thread_join(&thread, -1));
		printf("server thread terminated\n");
	} catch(trn::ResultError e) {
		std::cout << "caught ResultError: " << e.what() << std::endl;
		fatal_init();
		fatal_transition_to_fatal_error(e.code.code, 0);
	}
	
	return 0;
}

namespace twili {

Twili::Twili() :
	event_waiter(),
	server(ResultCode::AssertOk(trn::ipc::server::IPCServer::Create(&event_waiter))),
	usb_bridge(this, std::make_shared<bridge::ITwibDeviceInterface>(0, *this)) {
	
	server.CreateService("twili", [this](auto s) {
			return new twili::ITwiliService(this);
		});

	auto hbabi_shim_nro = util::ReadFile("/squash/hbabi_shim.nro");
	if(!hbabi_shim_nro) {
		throw trn::ResultError(TWILI_ERR_IO_ERROR);
	}
	this->hbabi_shim_nro = *hbabi_shim_nro;
	
	printf("initialized Twili\n");
}

std::optional<twili::MonitoredProcess*> Twili::FindMonitoredProcess(uint64_t pid) {
   auto i = std::find_if(monitored_processes.begin(),
                         monitored_processes.end(),
                         [pid](auto const &proc) {
                            return proc.pid == pid;
                         });
   if(i == monitored_processes.end()) {
      return {};
   } else {
      return &(*i);
   }
}

}
