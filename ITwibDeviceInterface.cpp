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

#include "msgpack11/msgpack11.hpp"

#include "twili.hpp"
#include "err.hpp"
#include "process_creation.hpp"

using ResultCode = trn::ResultCode;
template<typename T>
using Result = trn::Result<T>;

namespace twili {
namespace bridge {

ITwibDeviceInterface::ITwibDeviceInterface(uint32_t device_id, Twili &twili) : Object(device_id), twili(twili) {
}

void ITwibDeviceInterface::HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
	trn::Result<std::nullopt_t> r = std::nullopt;
	switch((protocol::ITwibDeviceInterface::Command) command_id) {
	case protocol::ITwibDeviceInterface::Command::RUN:
		r = Run(payload, opener);
		break;
	case protocol::ITwibDeviceInterface::Command::REBOOT:
		r = Reboot(payload, opener);
		break;
	case protocol::ITwibDeviceInterface::Command::COREDUMP:
		r = CoreDump(payload, opener);
		break;
	case protocol::ITwibDeviceInterface::Command::TERMINATE:
		r = Terminate(payload, opener);
		break;
	case protocol::ITwibDeviceInterface::Command::LIST_PROCESSES:
		r = ListProcesses(payload, opener);
		break;
	case protocol::ITwibDeviceInterface::Command::UPGRADE_TWILI:
		r = UpgradeTwili(payload, opener);
		break;
	case protocol::ITwibDeviceInterface::Command::IDENTIFY:
		r = Identify(payload, opener);
		break;
	default:
		r = tl::make_unexpected(ResultCode(TWILI_ERR_PROTOCOL_UNRECOGNIZED_FUNCTION));
		break;
	}
	if(!r) {
		opener.BeginError(r.error(), 0);
	}
}

Result<std::nullopt_t> ITwibDeviceInterface::Reboot(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
	ResultCode::AssertOk(bpc_init());
	ResultCode::AssertOk(bpc_reboot_system());
	return std::nullopt;
}

Result<std::nullopt_t> ITwibDeviceInterface::Run(std::vector<uint8_t> nro, usb::USBBridge::ResponseOpener opener) {
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
	process_creation::ProcessBuilder::VectorDataReader hbabi_shim_reader(twili.hbabi_shim_nro);
	process_creation::ProcessBuilder::VectorDataReader nro_reader(nro);
	Result<uint64_t>   shim_addr = builder.AppendNRO(hbabi_shim_reader);
	if(!  shim_addr) { return tl::make_unexpected(  shim_addr.error()); }
	Result<uint64_t> target_addr = builder.AppendNRO(nro_reader);
	if(!target_addr) { return tl::make_unexpected(target_addr.error()); }
	
	auto proc = builder.Build();
	if(!proc) { return tl::make_unexpected(proc.error()); }
	
	auto mon = twili.monitored_processes.emplace_back(&twili, *proc, *target_addr);
	mon.Launch();
	
	auto writer = ResultCode::AssertOk(opener.BeginOk(sizeof(uint64_t)));
	writer.Write<uint64_t>(mon.pid);
	return std::nullopt;
}

Result<std::nullopt_t> ITwibDeviceInterface::CoreDump(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
    if (payload.size() != sizeof(uint64_t)) {
        return tl::make_unexpected(TWILI_ERR_BAD_REQUEST);
    }
    uint64_t pid = *((uint64_t*) payload.data());
    auto proc = twili.FindMonitoredProcess(pid);
    ELFCrashReport report;
    if (!proc) {
	    printf("generating crash report for non-monitored process 0x%lx...\n", pid);
	    return Process(pid).GenerateCrashReport(report, opener);
    } else {
	    printf("generating crash report for monitored process 0x%lx...\n", pid);
	    return (*proc)->GenerateCrashReport(report, opener);
    }
}

Result<std::nullopt_t> ITwibDeviceInterface::Terminate(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
   if(payload.size() != sizeof(uint64_t)) {
      return tl::make_unexpected(TWILI_ERR_BAD_REQUEST);
   }
   uint64_t pid = *((uint64_t*) payload.data());
   auto proc = twili.FindMonitoredProcess(pid);
   if(!proc) {
      return tl::make_unexpected(TWILI_ERR_UNRECOGNIZED_PID);
   } else {
      return (*proc)->Terminate();
   }
}

Result<std::nullopt_t> ITwibDeviceInterface::ListProcesses(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
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

	uint64_t my_pid;
	auto pr = trn::svc::GetProcessId(0xffff8001);
	if(!pr) { return tl::make_unexpected(r.error()); }
	my_pid = *pr;
	
	auto writer = ResultCode::AssertOk(opener.BeginOk(sizeof(ProcessReport) * num_pids));
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
			break;
		}
	}
	return std::nullopt;
}

Result<std::nullopt_t> ITwibDeviceInterface::Identify(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
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
		{"protocol", protocol::VERSION},
		{"firmware_version", firmware_version},
		{"serial_number", serial_number},
		{"bluetooth_bd_address", bluetooth_bd_address},
		{"wireless_lan_mac_address", wireless_lan_mac_address},
		{"device_nickname", std::string((char*) device_nickname.data())},
		{"mii_author_id", mii_author_id}
	};
	std::string ser = ident.dump();
	return ResultCode::AssertOk(opener.BeginOk(ser.size())).Write(ser);
}

Result<std::nullopt_t> ITwibDeviceInterface::UpgradeTwili(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
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
	twili.destroy_flag = true;
	return std::nullopt;
}

} // namespace bridge
} // namespace twili
