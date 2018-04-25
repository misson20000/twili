typedef bool _Bool;
#include<iostream>

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>
#include<libtransistor/thread.h>
#include<libtransistor/ipc/sm.h>
#include<libtransistor/ipc.h>
#include<libtransistor/ipc_helpers.h>
#include<libtransistor/loader_config.h>

#include<stdio.h>

#include "twili.hpp"
#include "ITwiliService.hpp"

twili::TwiliState twili::twili_state;

using ResultCode = Transistor::ResultCode;

void server_thread(void *arg) {
	Transistor::IPCServer::IPCServer *server = (Transistor::IPCServer::IPCServer*) arg;
	while(!twili::twili_state.destroy_server_flag) {
		server->Process(3000000000);
	}
}

int main() {
	uint64_t syscall_hints[2] = {0xffffffffffffffff, 0xffffffffffffffff};
	memcpy(loader_config.syscall_hints, syscall_hints, sizeof(syscall_hints));
	
	try {
		Transistor::IPCServer::IPCServer server = ResultCode::AssertOk(Transistor::IPCServer::IPCServer::Create());
		server.CreateService<twili::ITwiliService>("twili");
		
		trn_thread_t thread;
		ResultCode::AssertOk(trn_thread_create(&thread, server_thread, &server, 58, -2, 1024 * 64, NULL));
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
	}
		
	return 0;
}
