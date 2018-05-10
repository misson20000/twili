typedef bool _Bool;
#include<iostream>

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>
#include<libtransistor/thread.h>
#include<libtransistor/ipc/fatal.h>
#include<libtransistor/ipc/sm.h>
#include<libtransistor/ipc/bpc.h>
#include<libtransistor/ipc.h>
#include<libtransistor/ipc_helpers.h>
#include<libtransistor/loader_config.h>
#include<libtransistor/usb_serial.h>
#include<libtransistor/svc.h>

#include<unistd.h>
#include<stdio.h>

#include "util.hpp"
#include "twili.hpp"
#include "process_creation.hpp"
#include "ITwiliService.hpp"
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
		printf("server destroyed\n");
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
	usb_bridge(this) {
	
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

Result<std::nullopt_t> Twili::Reboot(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer) {
	ResultCode::AssertOk(bpc_init());
	ResultCode::AssertOk(bpc_reboot_system());
	return std::nullopt;
}

Result<std::nullopt_t> Twili::Run(std::vector<uint8_t> nro, usb::USBBridge::USBResponseWriter &writer) {
	std::vector<uint32_t> caps = {
		0b00011111111111111111111111101111, // SVC grants
		0b00111111111111111111111111101111,
		0b01011111111111111111111111101111,
		0b01100000000000001111111111101111,
		0b10011111100000000000000000001111,
		0b10100000000000000000111111101111,
		0b00000010000000000111001110110111, // KernelFlags
		0b00000000000000000101111111111111, // ApplicationType
		0b00000000000110000011111111111111, // KernelReleaseVersion
		0b00000010000000000111111111111111, // HandleTableSize
		0b00000000000000101111111111111111, // DebugFlags (can be debugged)
	};

	twili::process_creation::ProcessBuilder builder("twili_child", caps);
	Result<uint64_t>   shim_addr = builder.AppendNRO(hbabi_shim_nro);
	if(!  shim_addr) { return tl::make_unexpected(  shim_addr.error()); }
	Result<uint64_t> target_addr = builder.AppendNRO(nro);
	if(!target_addr) { return tl::make_unexpected(target_addr.error()); }
	
	auto proc = builder.Build();
	if(!proc) { return tl::make_unexpected(proc.error()); }
	
	auto mon = monitored_processes.emplace_back(this, *proc, *target_addr);
   mon.Launch();
   writer.BeginOk(sizeof(uint64_t));
   writer.Write<uint64_t>(mon.pid);
	return std::nullopt;
}

Result<std::nullopt_t> Twili::CoreDump(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer) {
	for(auto i = monitored_processes.begin(); i != monitored_processes.end(); i++) {
		if(i->crashed) {
			return i->CoreDump(writer);
		}
	}
	return tl::make_unexpected(TWILI_ERR_NO_CRASHED_PROCESSES);
}

Result<std::nullopt_t> Twili::Terminate(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer) {
   if(payload.size() != sizeof(uint64_t)) {
      return tl::make_unexpected(TWILI_ERR_BAD_REQUEST);
   }
   uint64_t pid = *((uint64_t*) payload.data());
   auto proc = FindMonitoredProcess(pid);
   if(!proc) {
      return tl::make_unexpected(TWILI_ERR_UNRECOGNIZED_PID);
   } else {
      return (*proc)->Terminate();
   }
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
