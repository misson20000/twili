#include<libtransistor/cpp/nx.hpp>

#include<stdio.h>
#include<unistd.h>

#include "applet_shim.hpp"
#include "err.hpp"

static uint8_t _heap[6 * 1024 * 1024];
extern "C" {
runconf_heap_mode_t _trn_runconf_heap_mode = _TRN_RUNCONF_HEAP_MODE_OVERRIDE;
void *_trn_runconf_heap_base = _heap;
size_t _trn_runconf_heap_size = sizeof(_heap);

uint64_t reg_backups[13];
uint64_t target_thunk(result_t (*entry)(loader_config_entry_t*, thread_h), loader_config_entry_t *config, thread_h thrd);
}

using namespace trn;

namespace twili {
namespace applet_shim {

void ControlMode(ipc::client::Object &iappletshim) {
	printf("AppletShim entering control mode\n");
	
	trn::service::SM sm = ResultCode::AssertOk(trn::service::SM::Initialize());

	ipc::client::Object iasaps = ResultCode::AssertOk(
		sm.GetService("appletAE")); // IAllSystemAppletProxiesService

	ipc_domain_t domain;
	ResultCode::AssertOk(ipc_convert_to_domain(&iasaps.object, &domain));

	ipc::client::Object ilap;
	ResultCode::AssertOk(
		iasaps.SendSyncRequest<200>( // OpenLibraryAppletProxyOld
			ipc::InPid(),
			ipc::InRaw<uint64_t>(0),
			ipc::InHandle<handle_t, ipc::copy>(0xffff8001),
			ipc::OutObject(ilap)));

	ipc::client::Object ilac;
	ResultCode::AssertOk(
		ilap.SendSyncRequest<11>( // GetLibraryAppletCreator
			ipc::OutObject(ilac)));
	
	Waiter waiter;
	KEvent event;
	ResultCode::AssertOk(
		iappletshim.SendSyncRequest<100>( // GetEvent
			ipc::OutHandle<KEvent, ipc::copy>(event)));
	
	auto wh = waiter.Add(
		event,
		[&iappletshim, &ilac, &event]() -> bool {
			printf("iappletshim controller spawning host\n");
			try {
				event.ResetSignal();
				
				uint32_t command;
				while(iappletshim.SendSyncRequest<101>(trn::ipc::OutRaw(command))) {
					printf("  command %d\n", command);
					
					ipc::client::Object ilaa;
					ResultCode::AssertOk(
						ilac.SendSyncRequest<0>( // CreateLibraryApplet
							ipc::InRaw<uint32_t>(0x15),
							ipc::InRaw<uint32_t>(0),
							ipc::OutObject(ilaa)));

					printf("created\n");
					
					ResultCode::AssertOk(
						ilaa.SendSyncRequest<10>()); // Start
					
					printf("started\n");
				}
			} catch(trn::ResultError &e) {
				printf("caught 0x%x\n", e.code.code);
				// TODO: report error back to Twili
			}
			
			return true;
		});

	printf("entering wait loop...\n");
	while(true) {
		ResultCode::AssertOk(waiter.Wait(3000000000));
	}
}

static void substitute_handle(ipc::client::Object &shimservice, handle_t *handle) {
	ResultCode::AssertOk(
		shimservice.SendSyncRequest<3>(
			ipc::InRaw(*(uint32_t*) handle),
			ipc::OutHandle<handle_t, ipc::copy>(*handle)));
}

void HostMode(ipc::client::Object &iappletshim) {
	printf("AppletShim entering host mode\n");
	
	size_t target_size;
	ResultCode::AssertOk(
		iappletshim.SendSyncRequest<200>( // GetTargetSize
			ipc::OutRaw(target_size)));

	printf("target is 0x%lx bytes long\n", target_size);
	
	as::Reservation target_source_res(target_size);
	as::Reservation target_dest_res(target_size);
	ResultCode::AssertOk(
		svcMapPhysicalMemory(target_source_res.base, target_size));

	printf("allocated memory at %p\n", target_source_res.base);
	printf("map region at %p\n", target_dest_res.base);
	
	ResultCode::AssertOk(
		iappletshim.SendSyncRequest<201>( // SetupTarget
			ipc::InRaw<uint64_t>((uint64_t) target_source_res.base),
			ipc::InRaw<uint64_t>((uint64_t) target_dest_res.base)));

	printf("twili has set up our target\n");
	
	ipc::client::Object shimservice;
	ResultCode::AssertOk(
		iappletshim.SendSyncRequest<202>( // OpenHBABIShim
			ipc::OutObject(shimservice)));

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

	entries.push_back(loader_config_entry_t {
			.key = LCONFIG_KEY_END_OF_LIST,
			.flags = LOADER_CONFIG_FLAG_RECOGNITION_MANDATORY
		});

	uint64_t target_entry_addr;
	ResultCode::AssertOk( // GetTargetEntryPoint()
		shimservice.SendSyncRequest<5>(
			ipc::OutRaw<uint64_t>(target_entry_addr)));
	
	result_t (*target_entry)(loader_config_entry_t*, thread_h) = (result_t (*)(loader_config_entry_t*, thread_h)) target_dest_res.base;

	printf("ready to jump to application\n");
	
	// make sure this is available for the application
	sm_force_finalize();
	
	// Run the application
	uint8_t tls_backup[0x200];
	memcpy(tls_backup, get_tls(), 0x200);
	result_t ret = target_thunk(target_entry, entries.data(), 0xFFFFFFFF);
	memcpy(get_tls(), tls_backup, 0x200);

	printf("application has returned\n");
	
	ResultCode::AssertOk(
		shimservice.SendSyncRequest<4>( // SetNextLoadPath
			ipc::Buffer<uint8_t, 0x5>(next_load_path, sizeof(next_load_path)),
			ipc::Buffer<uint8_t, 0x5>(next_load_argv, sizeof(next_load_argv))));

	ResultCode::AssertOk(
		shimservice.SendSyncRequest<6>( // SetExitCode
			ipc::InRaw<uint32_t>(ret)));

	ResultCode::AssertOk(
		iappletshim.SendSyncRequest<203>()); // FinalizeTarget
	
	ResultCode::AssertOk(
		svcUnmapPhysicalMemory(target_source_res.base, target_size));
}

} // namespace applet_shim
} // namespace twili

int main(int argc, char *argv[]) {
	try {
		twili_pipe_t twili_out;
		trn::ResultCode::AssertOk(twili_init());
		trn::ResultCode::AssertOk(twili_open_stdout(&twili_out));
		int fd = twili_pipe_fd(&twili_out);
		dup2(fd, STDOUT_FILENO); // stdout is available for debugging
		close(fd);
		printf("applet_shim says hello\n");

		trn::ResultCode::AssertOk(fatal_init());
		
		ipc::client::Object iappletshim;
		{
			service::SM sm = ResultCode::AssertOk(service::SM::Initialize());

			// connect to twili
			ipc::client::Object itwiliservice = 
				ResultCode::AssertOk(sm.GetService("twili"));

			ResultCode::AssertOk(
				itwiliservice.SendSyncRequest<4>( // OpenAppletShim
					ipc::InPid(),
					ipc::InHandle<handle_t, ipc::copy>(0xffff8001),
					ipc::OutObject(iappletshim)));
		}

		printf("got IAppletShim\n");
		
		twili::applet_shim::Mode mode;
		ResultCode::AssertOk(
			iappletshim.SendSyncRequest<0>( // GetMode
				ipc::OutRaw(mode)));

		switch(mode) {
		case twili::applet_shim::Mode::Control:
			twili::applet_shim::ControlMode(iappletshim);
		case twili::applet_shim::Mode::Host:
			twili::applet_shim::HostMode(iappletshim);
			break;
		default:
			printf("fatal: unknown mode %d\n", (uint32_t) mode);
			fatal_transition_to_fatal_error(TWILI_ERR_APPLET_SHIM_UNKNOWN_MODE, 0);
			break;
		}
		
		return 0;
	} catch(trn::ResultError &e) {
		printf("caught 0x%x\n", e.code.code);
		return e.code.code;
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
