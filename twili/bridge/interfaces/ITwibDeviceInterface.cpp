//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2018 misson20000 <xenotoad@xenotoad.net>
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

#include "ITwibDeviceInterface.hpp"

#include<array>

#include<libtransistor/cpp/ipcclient.hpp>
#include<libtransistor/cpp/ipc/sm.hpp>
#include<libtransistor/cpp/svc.hpp>
#include<libtransistor/ipc/fs/ifilesystem.h>
#include<libtransistor/ipc/fs/ifile.h>
#include<libtransistor/ipc/fs.h>
#include<libtransistor/ipc/bpc.h>
#include<libtransistor/util.h>
#include<libtransistor/svc.h>

#include "../../msgpack11/msgpack11.hpp"

#include "../../twili.hpp"
#include "../../Services.hpp"
#include "../../process/UnmonitoredProcess.hpp"
#include "../../ELFCrashReport.hpp"

#include "ITwibPipeReader.hpp"
#include "ITwibDebugger.hpp"
#include "ITwibProcessMonitor.hpp"
#include "ITwibFilesystemAccessor.hpp"

#include "err.hpp"

using namespace trn;

namespace twili {
namespace bridge {

ITwibDeviceInterface::ITwibDeviceInterface(uint32_t device_id, Twili &twili) : ObjectDispatcherProxy(*this, device_id), twili(twili), dispatcher(*this) {
}

void ITwibDeviceInterface::CreateMonitoredProcess(bridge::ResponseOpener opener, std::string type) {
	std::shared_ptr<process::MonitoredProcess> proc;
	if(type == "applet") {
		proc = twili.applet_tracker.CreateMonitoredProcess();
	} else if(type == "shell") {
		proc = twili.shell_tracker.CreateMonitoredProcess();
	} else {
		opener.RespondError(TWILI_ERR_UNRECOGNIZED_MONITORED_PROCESS_TYPE);
		return;
	}

	opener.RespondOk(opener.MakeObject<ITwibProcessMonitor>(proc));
}

void ITwibDeviceInterface::Reboot(bridge::ResponseOpener opener) {
	trn::service::SM sm = twili::Assert(trn::service::SM::Initialize());
	ipc::client::Object spsm = twili::Assert(sm.GetService("spsm"));

	opener.RespondOk();
	
	twili::Assert(
		spsm.SendSyncRequest<3>(
			ipc::InRaw<bool>(true)));
}

void ITwibDeviceInterface::CoreDump(bridge::ResponseOpener opener, uint64_t pid) {
	std::shared_ptr<process::Process> proc = twili.FindProcess(pid);
	ELFCrashReport report;
	report.Generate(*proc, opener);
}

void ITwibDeviceInterface::Terminate(bridge::ResponseOpener opener, uint64_t pid) {
	twili.FindProcess(pid)->Terminate();

	opener.RespondOk();
	return;
}

void ITwibDeviceInterface::ListProcesses(bridge::ResponseOpener opener) {
	uint64_t pids[256];
	uint32_t num_pids;
	twili::Assert(svcGetProcessList(&num_pids, pids, ARRAY_LENGTH(pids)));

	struct ProcessReport {
		uint64_t process_id;
		uint32_t result;
		uint64_t title_id;
		char process_name[12];
		uint32_t mmu_flags;
	};

	uint64_t my_pid = twili::Assert(trn::svc::GetProcessId(0xffff8001));
	
	auto writer = opener.BeginOk(sizeof(ProcessReport) * num_pids + sizeof(uint64_t));
	writer.Write<uint64_t>(num_pids); // maintain std::vector packing format
	for(uint32_t i = 0; i < num_pids; i++) {
		struct ProcessReport preport;
		preport.process_id = pids[i];
		preport.result = RESULT_OK;
		preport.title_id = 0;
		memset(preport.process_name, 0, sizeof(preport.process_name));
		preport.mmu_flags = 0;

		try {
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
		} catch(ResultError &e) {
			preport.result = e.code.code;
		}
		writer.Write(preport);
	}
	writer.Finalize();
}

void ITwibDeviceInterface::UpgradeTwili(bridge::ResponseOpener opener) {
	opener.RespondError(LIBTRANSISTOR_ERR_UNSPECIFIED);
}

void ITwibDeviceInterface::Identify(bridge::ResponseOpener opener) {
	printf("identifying...\n");
	trn::service::SM sm = twili::Assert(trn::service::SM::Initialize());
	trn::ipc::client::Object set_sys = twili::Assert(
		sm.GetService("set:sys"));
	trn::ipc::client::Object set_cal = twili::Assert(
		sm.GetService("set:cal"));

	std::vector<uint8_t> firmware_version(0x100);
	TWILI_BRIDGE_CHECK(twili::Unwrap(
		set_sys.SendSyncRequest<3>( // GetFirmwareVersion
			trn::ipc::Buffer<uint8_t, 0x1a>(firmware_version))));

	std::array<uint8_t, 0x18> serial_number;
	TWILI_BRIDGE_CHECK(twili::Unwrap(
		set_cal.SendSyncRequest<9>( // GetSerialNumber
			trn::ipc::OutRaw<std::array<uint8_t, 0x18>>(serial_number))));

	std::array<uint8_t, 6> bluetooth_bd_address;
	TWILI_BRIDGE_CHECK(twili::Unwrap(
		set_cal.SendSyncRequest<0>( // GetBluetoothBdAddress
			trn::ipc::OutRaw<std::array<uint8_t, 6>>(bluetooth_bd_address))));

	std::array<uint8_t, 6> wireless_lan_mac_address;
	TWILI_BRIDGE_CHECK(twili::Unwrap(
		set_cal.SendSyncRequest<6>( // GetWirelessLanMacAddress
			trn::ipc::OutRaw<std::array<uint8_t, 6>>(wireless_lan_mac_address))));

	std::vector<uint8_t> device_nickname(0x80);
	TWILI_BRIDGE_CHECK(twili::Unwrap(
		set_sys.SendSyncRequest<77>( // GetDeviceNickName
			trn::ipc::Buffer<uint8_t, 0x16>(device_nickname))));

	std::array<uint8_t, 16> mii_author_id;
	TWILI_BRIDGE_CHECK(twili::Unwrap(
		set_sys.SendSyncRequest<90>( // GetMiiAuthorId
			trn::ipc::OutRaw<std::array<uint8_t, 16>>(mii_author_id))));
	
	msgpack11::MsgPack ident = msgpack11::MsgPack::object {
		{"service", "twili"},
		{"protocol", protocol::VERSION},
		{"firmware_version", firmware_version},
		{"serial_number", std::string((char*) serial_number.data())},
		{"bluetooth_bd_address", bluetooth_bd_address},
		{"wireless_lan_mac_address", wireless_lan_mac_address},
		{"device_nickname", std::string((char*) device_nickname.data())},
		{"mii_author_id", mii_author_id}
	};

	opener.RespondOk(std::move(ident));
}

void ITwibDeviceInterface::ListNamedPipes(bridge::ResponseOpener opener) {
	std::vector<std::string> names;
	for(auto i : twili.named_pipes) {
		names.push_back(i.first);
	}
	opener.RespondOk(std::move(names));
}

void ITwibDeviceInterface::OpenNamedPipe(bridge::ResponseOpener opener, std::string name) {
	auto i = twili.named_pipes.find(name);
	if(i == twili.named_pipes.end()) {
		opener.RespondError(TWILI_ERR_NO_SUCH_PIPE);
		return;
	}
	
	opener.RespondOk(opener.MakeObject<ITwibPipeReader>(i->second));
}

void ITwibDeviceInterface::OpenActiveDebugger(bridge::ResponseOpener opener, uint64_t pid) {
	auto r = trn::svc::DebugActiveProcess(pid);

	if(r) {
		opener.RespondOk(opener.MakeObject<ITwibDebugger>(twili, std::move(*r), twili.FindMonitoredProcess(pid)));
	} else {
		opener.RespondError(r.error());
	}
}

void ITwibDeviceInterface::GetMemoryInfo(bridge::ResponseOpener opener) {
	size_t total_mem_available, total_mem_usage;

	TWILI_BRIDGE_CHECK(svcGetInfo(&total_mem_available, 6, 0xffff8001, 0));
	TWILI_BRIDGE_CHECK(svcGetInfo(&total_mem_usage,     7, 0xffff8001, 0));
	
	std::vector<msgpack11::MsgPack::object> resource_limit_infos;
	for(size_t i = 0; i < 3; i++) {
		hos_types::ResourceLimitInfo rl;
		TWILI_BRIDGE_CHECK(twili.services->GetCurrentLimitInfo(i, 0, &rl)); // LimitableResource_Memory
		
		resource_limit_infos.push_back({{"category", i}, {"current_value", rl.current_value}, {"limit_value", rl.limit_value}});
	}
	
	msgpack11::MsgPack meminfo = msgpack11::MsgPack::object {
		{"total_memory_available", total_mem_available},
		{"total_memory_usage", total_mem_usage},
		{"limits", resource_limit_infos},
	};

	opener.RespondOk(std::move(meminfo));
}

void ITwibDeviceInterface::PrintDebugInfo(bridge::ResponseOpener opener) {
	printf("Twili state dump:\n");
	printf("  monitored process list\n");
	for(auto proc : twili.monitored_processes) {
		auto ptr = proc.get();
		printf("    - %p\n", ptr);
		printf("      type: %s\n", typeid(*ptr).name());
		printf("      pid: 0x%lx\n", proc->GetPid());
		printf("      state: %d\n", proc->GetState());
		printf("      result: 0x%x\n", proc->GetResult().code);
		printf("      target entry: 0x%lx\n", proc->GetTargetEntry());
		printf("      detail:\n");
		proc->PrintDebugInfo("        ");
	}
	twili.applet_tracker.PrintDebugInfo();
	twili.shell_tracker.PrintDebugInfo();

	opener.RespondOk();
}

void ITwibDeviceInterface::LaunchUnmonitoredProcess(bridge::ResponseOpener opener, uint32_t flags, uint64_t tid, uint64_t storage) {
	uint64_t pid;
	TWILI_BRIDGE_CHECK(twili.services->LaunchProgram(flags, tid, storage, &pid));
	opener.RespondOk();
}

void ITwibDeviceInterface::OpenFilesystemAccessor(bridge::ResponseOpener opener, std::string fs) {
	ifilesystem_t ifs;
	
	class FspSrvHolder {
	 public:
		FspSrvHolder() { twili::Assert(fsp_srv_init(0)); }
		~FspSrvHolder() { fsp_srv_finalize(); }
	} fsp_srv_holder;

	if(fs == "sd") {
		TWILI_BRIDGE_CHECK(fsp_srv_mount_sd_card(&ifs));
	} else if(fs == "nand_user") {
		TWILI_BRIDGE_CHECK(fsp_srv_open_bis_filesystem(&ifs, 30, ""));
	} else if(fs == "nand_system") {
		TWILI_BRIDGE_CHECK(fsp_srv_open_bis_filesystem(&ifs, 31, ""));
	} else {
		opener.RespondError(TWILI_ERR_UNKNOWN_FILESYSTEM);
		return;
	}
	
	opener.RespondOk(opener.MakeObject<ITwibFilesystemAccessor>(ifs));
}

void ITwibDeviceInterface::WaitToDebugApplication(bridge::ResponseOpener opener) {
	/*
		AMS dmnt slurps up all the debug hooks... need to negotiate that.
	 */
	opener.RespondError(TWILI_ERR_TODO);
}

void ITwibDeviceInterface::WaitToDebugTitle(bridge::ResponseOpener opener, uint64_t tid) {
	TWILI_BRIDGE_CHECK(wh_debug_title ? TWILI_ERR_ALREADY_WAITING : RESULT_OK);
	TWILI_BRIDGE_CHECK(twili.services->HookToCreateProcess(tid, &ev_debug_title));
	
	wh_debug_title = twili.event_waiter.Add(
		ev_debug_title,
		[this, opener, tid]() mutable -> bool {
			uint64_t pid;
			trn::ResultCode r = twili.services->GetProcessId(tid, &pid);
			if(r != RESULT_OK) {
				opener.RespondError(r);
			} else {
				opener.RespondOk(std::move(pid));
			}
			wh_debug_title.reset();
			return false;
		});
}

void ITwibDeviceInterface::RebootUnsafe(bridge::ResponseOpener opener) {
	trn::service::SM sm = twili::Assert(trn::service::SM::Initialize());
	ipc::client::Object bpc_ams = twili::Assert(sm.GetService("bpc:ams"));

	opener.RespondOk();

	static uint8_t context[0x350] = {0};
	
	twili::Assert(
		bpc_ams.SendSyncRequest<65000>(
			ipc::Buffer<uint8_t, 0x15>(context, sizeof(context))));
}

} // namespace bridge
} // namespace twili
