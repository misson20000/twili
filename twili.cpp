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

#include "twili.hpp"
#include "process_creation.hpp"
#include "ITwiliService.hpp"
#include "USBBridge.hpp"

using ResultCode = Transistor::ResultCode;

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
			throw new Transistor::ResultError(-usb_fd);
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

		ipc_object_t pipe;
		{
			ipc_request_t rq = ipc_default_request;
			rq.request_id = 1;
			ipc_response_fmt_t rs = ipc_default_response_fmt;
			rs.num_objects = 1;
			rs.objects = &pipe;
			ResultCode::AssertOk(ipc_send(sess, &rq, &rs));
		}

		{
			ipc_request_t rq = ipc_default_request;
			rq.request_id = 1;
			ipc_buffer_t buffers[] = {
				ipc_buffer_from_string("hello, world, over twili IPC!\n", 0x5)
			};
			ipc_msg_set_buffers(rq, buffers, buffer_ptrs);
			ResultCode::AssertOk(ipc_send(pipe, &rq, &ipc_default_response_fmt));
		}

		//printf("destroying server...\n");
		//twili::twili_state.destroy_server_flag = true;
		
		ResultCode::AssertOk(trn_thread_join(&thread, -1));
		printf("server destroyed\n");
	} catch(Transistor::ResultError e) {
		std::cout << "caught ResultError: " << e.what() << std::endl;
		fatal_init();
		fatal_transition_to_fatal_error(e.code.code, 0);
	}
	
	return 0;
}

namespace twili {

Twili::Twili() :
	event_waiter(),
	server(ResultCode::AssertOk(Transistor::IPCServer::IPCServer::Create(&event_waiter))),
	usb_bridge(this) {
	
	server.CreateService("twili", [this](auto s) {
			return new twili::ITwiliService(this);
		});
	printf("initialized Twili\n");
}

bool Twili::Reboot() {
	ResultCode::AssertOk(bpc_init());
	ResultCode::AssertOk(bpc_reboot_system());
	return true;
}

bool Twili::Run(std::vector<uint8_t> nro) {
	monitored_processes.emplace_back(
		this,
		ResultCode::AssertOk(twili::process_creation::CreateProcessFromNRO(nro, "twili_child"))
	).Launch();
	return true;
}

}
