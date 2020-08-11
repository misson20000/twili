//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2019 misson20000 <xenotoad@xenotoad.net>
//
// This file is part of Twili.
//
// Twili is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Twili is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Twili.  If not, see <http://www.gnu.org/licenses/>.
//

#include<libtransistor/cpp/nx.hpp>

#include<stdio.h>
#include<unistd.h>

#include "err.hpp"

static uint8_t _heap[8 * 1024 * 1024];
extern "C" {
runconf_heap_mode_t _trn_runconf_heap_mode = _TRN_RUNCONF_HEAP_MODE_OVERRIDE;
void *_trn_runconf_heap_base = _heap;
size_t _trn_runconf_heap_size = sizeof(_heap);

uint64_t reg_backups[13];
uint64_t target_thunk(result_t (*entry)(loader_config_entry_t*, int64_t), loader_config_entry_t *config, int64_t thrd);
}

using namespace trn;

static void substitute_handle(ipc::client::Object &shimservice, handle_t *handle) {
	ResultCode::AssertOk(
		shimservice.SendSyncRequest<3>(
			ipc::InRaw(*(uint32_t*) handle),
			ipc::OutHandle<handle_t, ipc::copy>(*handle)));
}

int main(int argc, char *argv[]) {
	try {
		twili_pipe_t twili_out;
		trn::ResultCode::AssertOk(twili_init());
		trn::ResultCode::AssertOk(twili_open_stdout(&twili_out));
		int fd = twili_pipe_fd(&twili_out);
		dup2(fd, STDOUT_FILENO); // stdout is available for debugging
		close(fd);

		ResultCode::AssertOk(sm_init()); // make sure there's a reference by the time we reach sm_force_finalize()
		
		ipc::client::Object shimservice;
	
		{
			service::SM sm = ResultCode::AssertOk(service::SM::Initialize());

			// connect to twili
			ipc::client::Object itwiliservice = 
				ResultCode::AssertOk(sm.GetService("twili"));

			// open our IHBABIShim
			ResultCode::AssertOk(
				itwiliservice.SendSyncRequest<5>(
					ipc::InPid(),
					ipc::InHandle<handle_t, ipc::copy>(0xffff8001),
					ipc::OutObject<ipc::client::Object>(shimservice)));
		} // at this point we no longer need SM or ITwiliService
		sm_force_finalize();
		
		uint32_t num_loader_config_entries;
		ResultCode::AssertOk( // GetLoaderConfigEntryCount()
			shimservice.SendSyncRequest<1>(
				ipc::OutRaw(num_loader_config_entries)));
		std::vector<loader_config_entry_t> entries(num_loader_config_entries, loader_config_entry_t {});
		
		ResultCode::AssertOk( // GetLoaderConfigEntries()
			shimservice.SendSyncRequest<2>(
				ipc::Buffer<loader_config_entry_t, 0x6>(entries)));

		// Translate handles from Twili.
		// Twili can't inject handles into our process, so
		// it passes placeholders that we are expected to ask
		// Twili to exchange for actual handles.
		for(auto i = entries.begin(); i != entries.end(); i++) {
			switch(i->key) {
			case LCONFIG_KEY_OVERRIDE_SERVICE:
				substitute_handle(shimservice, &i->override_service.override.service_handle);
				break;
			case LCONFIG_KEY_PROCESS_HANDLE:
				substitute_handle(shimservice, &i->process_handle.process_handle);
				break;
			case LCONFIG_KEY_END_OF_LIST:
			case LCONFIG_KEY_NEXT_LOAD_PATH:
			case LCONFIG_KEY_OVERRIDE_HEAP:
			case LCONFIG_KEY_ARGV:
			case LCONFIG_KEY_SYSCALL_AVAILABLE_HINT:
			case LCONFIG_KEY_APPLET_TYPE:
			case LCONFIG_KEY_APPLET_WORKAROUND:
			case LCONFIG_KEY_STDIO_SOCKETS:
			case LCONFIG_KEY_LAST_LOAD_RESULT:
			case LCONFIG_KEY_ALLOC_PAGES:
			case LCONFIG_KEY_LOCK_REGION:
				break;
			default:
				if(i->flags & LOADER_CONFIG_FLAG_RECOGNITION_MANDATORY) {
					throw ResultError(HOMEBREW_ABI_UNRECOGNIZED_KEY(i->key));
				}
			}
		}

		// Twili can't give us this key
		entries.push_back(loader_config_entry_t {
				.key = LCONFIG_KEY_MAIN_THREAD_HANDLE,
				.flags = 0,
				.main_thread_handle = {loader_config.main_thread},
			});

		handle_t process_handle;
		ResultCode::AssertOk( // GetProcessHandle
			shimservice.SendSyncRequest<0>(
				ipc::OutHandle<handle_t, ipc::copy>(process_handle)));
		entries.push_back(loader_config_entry_t {
				.key = LCONFIG_KEY_PROCESS_HANDLE,
				.flags = 0,
				.process_handle = {process_handle},
			});
		
		// This key is also best handled by us, since Twili
		// would have a hard time reading these buffers back out.
		uint8_t next_load_path[512] = {0};
		uint8_t next_load_argv[2048] = {0};
		entries.push_back(loader_config_entry_t {
				.key = LCONFIG_KEY_NEXT_LOAD_PATH,
				.flags = 0,
				.next_load_path = {
					.nro_path = (char (*)[512]) &next_load_path,
					.argv_str = (char (*)[2048]) &next_load_argv
				}
			});

		// This key is also best handled by us, since Twili
		// would have a hard time sending this data otherwise.
		char current_argv[2048] = {0};
		ResultCode::AssertOk( // GetArgv
			shimservice.SendSyncRequest<8>(
				ipc::Buffer<char, 0x6>(current_argv, sizeof(current_argv))));
		entries.push_back(loader_config_entry_t {
				.key = LCONFIG_KEY_ARGV,
				.flags = 0,
				.argv = {
					.argv = (char**) current_argv
				}
			});

		entries.push_back(loader_config_entry_t {
				.key = LCONFIG_KEY_END_OF_LIST,
				.flags = LOADER_CONFIG_FLAG_RECOGNITION_MANDATORY
			});

		uint64_t target_entry_addr;
		ResultCode::AssertOk( // GetTargetEntryPoint()
			shimservice.SendSyncRequest<5>(
				ipc::OutRaw<uint64_t>(target_entry_addr)));

		result_t (*target_entry)(loader_config_entry_t*, int64_t) = (result_t (*)(loader_config_entry_t*, int64_t)) target_entry_addr;

		auto r = shimservice.SendSyncRequest<7>(); // WaitToStart()
		if(!r) {
			if(r.error().code == TWILI_ERR_NO_LONGER_REQUESTED_TO_LAUNCH) {
				// exit cleanly
				return 0;
			} else {
				ResultCode::AssertOk(r.error().code);
			}
		}
		
		// Run the application
		uint8_t tls_backup[0x200];
		memcpy(tls_backup, get_tls(), 0x200);
		result_t ret = target_thunk(target_entry, entries.data(), -1);
		memcpy(get_tls(), tls_backup, 0x200);

		ResultCode::AssertOk(
			shimservice.SendSyncRequest<4>( // SetNextLoadPath
				ipc::Buffer<uint8_t, 0x5>(next_load_path, sizeof(next_load_path)),
				ipc::Buffer<uint8_t, 0x5>(next_load_argv, sizeof(next_load_argv))));

		ResultCode::AssertOk(
			shimservice.SendSyncRequest<6>( // SetExitCode
				ipc::InRaw<uint32_t>(ret)));
	} catch(trn::ResultError &e) {
		// crash and generate core dump
		printf("caught %s\n", e.what());
		svcBreak(e.code.code, 0, 0);
	}

	return 0;
}

asm(".text\n"
	".globl target_thunk\n"
"target_thunk:"
	"adrp x16, reg_backups\n"
	"add x16, x16, #:lo12:reg_backups\n"
	"mov x17, sp\n"
	"stp x19, x20, [x16, 0]\n"
	"stp x21, x22, [x16, 0x10]\n"
	"stp x23, x24, [x16, 0x20]\n"
	"stp x25, x26, [x16, 0x30]\n"
	"stp x27, x28, [x16, 0x40]\n"
	"stp x29, x30, [x16, 0x50]\n"
	"str x17, [x16, 0x60]\n"
	"mov x8, x0\n"
	"mov x0, x1\n"
	"mov x1, x2\n"
	"blr x8\n"
	"adrp x16, reg_backups\n"
	"add x16, x16, #:lo12:reg_backups\n"
	"ldp x19, x20, [x16, 0]\n"
	"ldp x21, x22, [x16, 0x10]\n"
	"ldp x23, x24, [x16, 0x20]\n"
	"ldp x25, x26, [x16, 0x30]\n"
	"ldp x27, x28, [x16, 0x40]\n"
	"ldp x29, x30, [x16, 0x50]\n"
	"ldr x17, [x16, 0x60]\n"
	"mov sp, x17\n"
	"ret");
