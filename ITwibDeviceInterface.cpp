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
#include "ITwibPipeReader.hpp"

using trn::ResultCode;
using trn::ResultError;

template<typename T>
using Result = trn::Result<T>;

namespace twili {
namespace bridge {

ITwibDeviceInterface::ITwibDeviceInterface(uint32_t device_id, Twili &twili) : Object(device_id), twili(twili) {
}

void ITwibDeviceInterface::HandleRequest(uint32_t command_id, std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
	switch((protocol::ITwibDeviceInterface::Command) command_id) {
	case protocol::ITwibDeviceInterface::Command::RUN:
		Run(payload, opener);
		break;
	case protocol::ITwibDeviceInterface::Command::REBOOT:
		Reboot(payload, opener);
		break;
	case protocol::ITwibDeviceInterface::Command::COREDUMP:
		CoreDump(payload, opener);
		break;
	case protocol::ITwibDeviceInterface::Command::TERMINATE:
		Terminate(payload, opener);
		break;
	case protocol::ITwibDeviceInterface::Command::LIST_PROCESSES:
		ListProcesses(payload, opener);
		break;
	case protocol::ITwibDeviceInterface::Command::UPGRADE_TWILI:
		UpgradeTwili(payload, opener);
		break;
	case protocol::ITwibDeviceInterface::Command::IDENTIFY:
		Identify(payload, opener);
		break;
	case protocol::ITwibDeviceInterface::Command::LIST_NAMED_PIPES:
		ListNamedPipes(payload, opener);
		break;
	case protocol::ITwibDeviceInterface::Command::OPEN_NAMED_PIPE:
		OpenNamedPipe(payload, opener);
		break;
	default:
		opener.BeginError(ResultCode(TWILI_ERR_PROTOCOL_UNRECOGNIZED_FUNCTION), 0);
		break;
	}
}

void ITwibDeviceInterface::Reboot(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
	ResultCode::AssertOk(bpc_init());
	ResultCode::AssertOk(bpc_reboot_system());
}

void ITwibDeviceInterface::Run(std::vector<uint8_t> nro, usb::USBBridge::ResponseOpener opener) {
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
	uint64_t   shim_addr = ResultCode::AssertOk(builder.AppendNRO(hbabi_shim_reader));
	uint64_t target_addr = ResultCode::AssertOk(builder.AppendNRO(nro_reader));
	auto            proc = ResultCode::AssertOk(builder.Build());
	
	auto mon = twili.monitored_processes.emplace_back(&twili, proc, target_addr);
	mon.Launch();

	struct {
		uint64_t pid;
		uint32_t tp_stdin;
		uint32_t tp_stdout;
		uint32_t tp_stderr;
	} response;

	response.pid = mon.pid;
	//response.tp_stdin = opener.MakeObject<>(mon.tp_stdin);
	response.tp_stdout = opener.MakeObject<ITwibPipeReader>(mon.tp_stdout)->object_id;
	response.tp_stderr = opener.MakeObject<ITwibPipeReader>(mon.tp_stderr)->object_id;
	
	opener.BeginOk(sizeof(response)).Write<decltype(response)>(response);
}

void ITwibDeviceInterface::CoreDump(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
	if(payload.size() != sizeof(uint64_t)) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}
	uint64_t pid = *((uint64_t*) payload.data());
	auto proc = twili.FindMonitoredProcess(pid);
	ELFCrashReport report;
	if (!proc) {
		printf("generating crash report for non-monitored process 0x%lx...\n", pid);
		Process(pid).GenerateCrashReport(report, opener);
	} else {
		printf("generating crash report for monitored process 0x%lx...\n", pid);
		(*proc)->GenerateCrashReport(report, opener);
	}
}

void ITwibDeviceInterface::Terminate(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
	if(payload.size() != sizeof(uint64_t)) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}
	uint64_t pid = *((uint64_t*) payload.data());
	auto proc = twili.FindMonitoredProcess(pid);
	if(!proc) {
		throw ResultError(TWILI_ERR_UNRECOGNIZED_PID);
	} else {
		(*proc)->Terminate();
	}
}

void ITwibDeviceInterface::ListProcesses(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
	uint64_t pids[256];
	uint32_t num_pids;
	ResultCode::AssertOk(svcGetProcessList(&num_pids, pids, ARRAY_LENGTH(pids)));

	struct ProcessReport {
		uint64_t process_id;
		uint32_t result;
		uint64_t title_id;
		char process_name[12];
		uint32_t mmu_flags;
	};

	uint64_t my_pid = ResultCode::AssertOk(trn::svc::GetProcessId(0xffff8001));
	
	auto writer = opener.BeginOk(sizeof(ProcessReport) * num_pids);
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
}

void ITwibDeviceInterface::Identify(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
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
	opener.BeginOk(ser.size()).Write(ser);
}

void ITwibDeviceInterface::UpgradeTwili(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
	printf("no-op\n");
}

void ITwibDeviceInterface::ListNamedPipes(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
	if(payload.size() != 0) {
		throw ResultError(TWILI_ERR_BAD_REQUEST);
	}

	size_t size = 0;
	size+= sizeof(uint32_t);
	for(auto i : twili.named_pipes) {
		size+= sizeof(uint32_t);
		size+= i.first.size();
	}

	auto w = opener.BeginOk(size);
	w.Write<uint32_t>(twili.named_pipes.size());
	for(auto i : twili.named_pipes) {
		w.Write<uint32_t>(i.first.size());
		w.Write((uint8_t*) i.first.data(), i.first.size());
	}
}

void ITwibDeviceInterface::OpenNamedPipe(std::vector<uint8_t> payload, usb::USBBridge::ResponseOpener opener) {
	std::string name(payload.begin(), payload.end());

	auto i = twili.named_pipes.find(name);
	if(i == twili.named_pipes.end()) {
		throw ResultError(TWILI_ERR_NO_SUCH_PIPE);
	}
	
	uint32_t id = opener.MakeObject<ITwibPipeReader>(i->second)->object_id;
	opener.BeginOk(sizeof(id)).Write(id);
}

} // namespace bridge
} // namespace twili
