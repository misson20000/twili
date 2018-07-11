typedef bool _Bool;
#include<iostream>
#include<array>

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>
#include<libtransistor/cpp/ipcclient.hpp>
#include<libtransistor/cpp/ipc/sm.hpp>
#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/thread.h>
#include<libtransistor/ipc/fs/ifilesystem.h>
#include<libtransistor/ipc/fs/ifile.h>
#include<libtransistor/ipc/fs.h>
#include<libtransistor/ipc/fatal.h>
#include<libtransistor/ipc/sm.h>
#include<libtransistor/ipc/bpc.h>
#include<libtransistor/ipc.h>
#include<libtransistor/ipc_helpers.h>
#include<libtransistor/loader_config.h>
#include<libtransistor/runtime_config.h>
#include<libtransistor/usb_serial.h>
#include<libtransistor/svc.h>

#include<unistd.h>
#include<stdio.h>

#include "util.hpp"
#include "twili.hpp"
#include "process_creation.hpp"
#include "Process.hpp"
#include "ITwiliService.hpp"
#include "USBBridge.hpp"
#include "err.hpp"

#include "msgpack11/msgpack11.hpp"

uint8_t _local_heap[32 * 1024 * 1024];

runconf_heap_mode_t _trn_runconf_heap_mode = _TRN_RUNCONF_HEAP_MODE_OVERRIDE;
void *_trn_runconf_heap_base = _local_heap;
size_t _trn_runconf_heap_size = sizeof(_local_heap);

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
		0b10011111111111111111111111101111,
		0b10100000000000000000111111101111,
		0b00000010000000000111001110110111, // KernelFlags
		0b00000000000000000101111111111111, // ApplicationType
		0b00000000000110000011111111111111, // KernelReleaseVersion
		0b00000010000000000111111111111111, // HandleTableSize
		0b00000000000001101111111111111111, // DebugFlags (can be debugged)
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
    if (payload.size() != sizeof(uint64_t)) {
        return tl::make_unexpected(TWILI_ERR_BAD_REQUEST);
    }
    uint64_t pid = *((uint64_t*) payload.data());
    auto proc = FindMonitoredProcess(pid);
    ELFCrashReport report;
    if (!proc) {
	    printf("generating crash report for non-monitored process 0x%lx...\n", pid);
	    return Process(pid).GenerateCrashReport(report, writer);
    } else {
	    printf("generating crash report for monitored process 0x%lx...\n", pid);
	    return (*proc)->GenerateCrashReport(report, writer);
    }
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

Result<std::nullopt_t> Twili::ListProcesses(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer) {
	uint64_t pids[256];
	uint32_t num_pids;
	auto r = ResultCode::ExpectOk(svcGetProcessList(&num_pids, pids, ARRAY_LENGTH(pids)));
	if(!r) {
		return r;
	}

	struct ProcessReport {
		uint64_t process_id;
		uint32_t result;
		uint64_t title_id;
		char process_name[12];
		uint32_t mmu_flags;
	};

	{
		auto r = writer.BeginOk(sizeof(ProcessReport) * num_pids);
		if(!r) {
			return r;
		}
	}

	uint64_t my_pid;
	auto pr = trn::svc::GetProcessId(0xffff8001);
	if(!pr) { return tl::make_unexpected(r.error()); }
	my_pid = *pr;
	
	for(uint32_t i = 0; i < num_pids; i++) {
		struct ProcessReport preport;
		preport.process_id = pids[i];
		preport.result = RESULT_OK;
		preport.title_id = 0;
		memset(preport.process_name, 0, sizeof(preport.process_name));
		preport.mmu_flags = 0;
		if(pids[i] == my_pid) {
			preport.result = TWILI_ERR_WONT_DEBUG_SELF;
		} else {
			auto dr = trn::svc::DebugActiveProcess(pids[i]);
			if(!dr) {
				preport.result = dr.error().code;
			} else {
				trn::KDebug debug = std::move(*dr);
				auto er = trn::svc::GetDebugEvent(debug);
				while(er) {
					if(er->event_type == DEBUG_EVENT_ATTACH_PROCESS) {
						preport.title_id = er->attach_process.title_id;
						memcpy(preport.process_name, er->attach_process.process_name, 12);
						preport.mmu_flags = er->attach_process.mmu_flags;
					}
					er = trn::svc::GetDebugEvent(debug);
				}
				if(er.error().code != 0x8c01) {
					preport.result = er.error().code;
				}
			}
		}
		auto r = writer.Write(preport);
		if(!r) {
			return r;
		}
	}

	return std::nullopt;
}

Result<std::nullopt_t> Twili::Identify(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer) {
	printf("identifying...\n");
	trn::service::SM sm = ResultCode::AssertOk(trn::service::SM::Initialize());
	trn::ipc::client::Object set_sys = ResultCode::AssertOk(
		sm.GetService("set:sys"));
	trn::ipc::client::Object set_cal = ResultCode::AssertOk(
		sm.GetService("set:cal"));

	std::vector<uint8_t> firmware_version(0x100);
	ResultCode::AssertOk(
		set_sys.SendSyncRequest<3>( // GetFirmwareVersion
			trn::ipc::Buffer<uint8_t, 0x1a>(firmware_version)));

	std::array<uint8_t, 0x18> serial_number;
	ResultCode::AssertOk(
		set_cal.SendSyncRequest<9>( // GetSerialNumber
			trn::ipc::OutRaw<std::array<uint8_t, 0x18>>(serial_number)));

	std::array<uint8_t, 6> bluetooth_bd_address;
	ResultCode::AssertOk(
		set_cal.SendSyncRequest<0>( // GetBluetoothBdAddress
			trn::ipc::OutRaw<std::array<uint8_t, 6>>(bluetooth_bd_address)));

	std::array<uint8_t, 6> wireless_lan_mac_address;
	ResultCode::AssertOk(
		set_cal.SendSyncRequest<6>( // GetWirelessLanMacAddress
			trn::ipc::OutRaw<std::array<uint8_t, 6>>(wireless_lan_mac_address)));

	std::vector<uint8_t> device_nickname(0x80);
	ResultCode::AssertOk(
		set_sys.SendSyncRequest<77>( // GetDeviceNickName
			trn::ipc::Buffer<uint8_t, 0x16>(device_nickname)));

	std::array<uint8_t, 16> mii_author_id;
	ResultCode::AssertOk(
		set_sys.SendSyncRequest<90>( // GetMiiAuthorId
			trn::ipc::OutRaw<std::array<uint8_t, 16>>(mii_author_id)));
	
	msgpack11::MsgPack ident = msgpack11::MsgPack::object {
		{"service", "twili"},
		{"protocol", twili::usb::USBBridge::PROTOCOL_VERSION},
		{"firmware_version", firmware_version},
		{"serial_number", serial_number},
		{"bluetooth_bd_address", bluetooth_bd_address},
		{"wireless_lan_mac_address", wireless_lan_mac_address},
		{"device_nickname", std::string((char*) device_nickname.data())},
		{"mii_author_id", mii_author_id}
	};
	std::string ser = ident.dump();
	auto r = writer.BeginOk(ser.size());
	if(!r) { return r; }
	return writer.Write(ser);
}

Result<std::nullopt_t> Twili::UpgradeTwili(std::vector<uint8_t> payload, usb::USBBridge::USBResponseWriter &writer) {
	printf("upgrading twili...\n");
	ifilesystem_t user_ifs;
	auto r = ResultCode::ExpectOk(fsp_srv_open_bis_filesystem(&user_ifs, 30, ""));
	if(!r) { return r; }
	printf("opened bis@User filesystem\n");

	uint8_t path[0x301];
	strcpy((char*) path, "/twili_launcher.nsp");
	
	ifilesystem_delete_file(user_ifs, path); // allow failure
	printf("deleted existing %s\n", path);
	r = ResultCode::ExpectOk(
		ifilesystem_create_file(user_ifs, 0, payload.size(), path));
	if(!r) { ipc_close(user_ifs); return r; }
	printf("created new %s\n", path);
	
	ifile_t file;
	r = ResultCode::ExpectOk(
		ifilesystem_open_file(user_ifs, &file, 6, path));
	if(!r) { ipc_close(user_ifs); return r; }

	printf("writing new Twili launcher...\n");
	r = ResultCode::ExpectOk(
		ifile_write(file, 0, 0, payload.size(), (int8_t*) payload.data(), payload.size()));
	if(!r) {
		ipc_close(file);
		ipc_close(user_ifs);
		return r;
	}
	printf("wrote new Twili launcher\n");

	r = ResultCode::ExpectOk(ipc_close(file));
	if(!r) { ipc_close(user_ifs); return r; }
	
	r = ResultCode::ExpectOk(
		ifilesystem_commit(user_ifs));
	if(!r) { ipc_close(user_ifs); return r; }
	printf("committed changes\n");

	r = ResultCode::ExpectOk(ipc_close(user_ifs));
	if(!r) { return r; }

	printf("killing server...\n");
	printf("  (you will have to manually relaunch Twili)\n");
	destroy_flag = true;
	return std::nullopt;
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
