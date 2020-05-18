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

typedef bool _Bool;
#include<iostream>

#include<libtransistor/cpp/types.hpp>
#include<libtransistor/cpp/ipcserver.hpp>
#include<libtransistor/cpp/ipcclient.hpp>
#include<libtransistor/cpp/ipc/sm.hpp>
#include<libtransistor/ipc/fatal.h>
#include<libtransistor/ipc/sm.h>
#include<libtransistor/ipc.h>
#include<libtransistor/ipc_helpers.h>
#include<libtransistor/loader_config.h>
#include<libtransistor/runtime_config.h>
#include<libtransistor/usb_serial.h>
#include<libtransistor/svc.h>

#include<unistd.h>
#include<stdio.h>

#include "inih/INIReader.h"

#include "util.hpp"
#include "twili.hpp"
#include "SystemVersion.hpp"
#include "Services.hpp"
#include "process_creation.hpp"
#include "process/Process.hpp"
#include "process/UnmonitoredProcess.hpp"
#include "service/ITwiliService.hpp"
#include "bridge/usb/USBBridge.hpp"
#include "bridge/tcp/TCPBridge.hpp"
#include "bridge/interfaces/ITwibDeviceInterface.hpp"
#include "err.hpp"

using namespace trn;

int main() {
	uint64_t syscall_hints[2] = {0xffffffffffffffff, 0xffffffffffffffff};
	memcpy(loader_config.syscall_hints, syscall_hints, sizeof(syscall_hints));
	
	try {
		twili::Twili::Config config;

		if(config.enable_usb_log) {
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
		}

		twili::SystemVersion::SetCurrent();
		auto v = twili::SystemVersion::Current();
		printf("running %d.%d.%d\n", v.major, v.minor, v.micro);
		
		switch(config.state) {
		case twili::Twili::Config::State::Fresh:
			printf("using fresh config\n");
			break;
		case twili::Twili::Config::State::Loaded:
			printf("using config from file\n");
			break;
		case twili::Twili::Config::State::Error:
			printf("error loading config (line %d)\n", config.error_line);
			break;
		}
		
		// initialize twili
		static twili::Twili twili(config);
		
		while(!twili.destroy_flag) {
			ResultCode::AssertOk(twili.event_waiter.Wait(3000000000));
			twili.monitored_processes.remove_if(
				[](const auto &proc) {
					return proc->GetState() == twili::process::MonitoredProcess::State::Exited;
				});
		}
		
		printf("twili terminating...\n");
		printf("terminating monitored processes...\n");
		for(std::shared_ptr<twili::process::MonitoredProcess> proc : twili.monitored_processes) {
			proc->Terminate();
		}
		printf("done\n");
	} catch(trn::ResultError &e) {
		std::cout << "caught ResultError: " << e.what() << std::endl;
		printf("  backtrace:\n");
		uint64_t aslr_base = (uint64_t) env_get_aslr_base();
		for(size_t i = 0; i < e.backtrace_size; i++) {
			uint64_t addr = e.backtrace[i];
			if(addr >= aslr_base && addr < aslr_base + 0x10000000) {
				printf("    - twili + 0x%lx\n", addr - aslr_base);
			} else {
				printf("    - 0x%lx\n", addr);
			}
		}
		twili::Abort(e);
	}
	
	return 0;
}

namespace twili {

void Abort(trn::ResultError &e) {
	trn::service::SM sm = ResultCode::AssertOk(trn::service::SM::Initialize());
   
	ipc::client::Object fatal = ResultCode::AssertOk(
		sm.GetService("fatal:u"));

	uint64_t policy = 0; // ErrorReportAndErrorScreen
	if(env_get_kernel_version() >= KERNEL_VERSION_300) {
		policy = 2; // ErrorScreen
	}

	struct FatalContext {
		uint64_t x[31] = {0};
		uint64_t sp = 0;
		uint64_t pc = 0;
		uint64_t pstate = 0;
		uint64_t afsr0 = 0;
		uint64_t afsr1 = 0;
		uint64_t esr = 0;
		uint64_t far = 0;

		uint64_t stack_trace[32] = {0};
		uint64_t start_address = 0;
		uint64_t register_set_flags = 0;
		uint32_t stack_trace_size = 0;

		bool is_aarch32 = false;
		uint32_t type = 0;
	} ctx;

	ctx.start_address = (uint64_t) env_get_aslr_base();
	ctx.stack_trace_size = std::min<size_t>(32, e.backtrace_size);
	std::copy_n(e.backtrace, ctx.stack_trace_size, ctx.stack_trace);
	
	fatal.SendSyncRequest<2>( // ThrowFatalWithCpuContext
		ipc::InRaw<uint32_t>(e.code.code),
		ipc::InRaw<uint32_t>(policy),
		ipc::InRaw<uint64_t>(0),
		ipc::InPid(),
		ipc::Buffer<FatalContext, 0x15>(&ctx, sizeof(ctx)));

	svcExitProcess();
}

void Abort(trn::ResultCode code) {
	trn::ResultError e(code);
	Abort(e);
}

Twili::Twili(const Config &config) :
	config(config),
	event_waiter(),
	server(ResultCode::AssertOk(trn::ipc::server::IPCServer::Create(&event_waiter))),
	twili_registration(
		server, config.service_name.c_str(),
		[this](auto s) {
			return new twili::service::ITwiliService(this);
		}),
	services(twili::Services::CreateForSystemVersion(twili::SystemVersion::Current())),
	file_manager(*this),
	applet_tracker(*this),
	shell_tracker(*this) {
	printf("finished constructing most members\n");
	//if(config.enable_usb_bridge) {
	//	usb_bridge = std::make_unique<bridge::usb::USBBridge>(this, std::make_shared<bridge::ITwibDeviceInterface>(0, *this));
	//	printf("constructed USB bridge\n");
	//}
	if(config.enable_tcp_bridge) {
		tcp_bridge = std::make_unique<bridge::tcp::TCPBridge>(*this, std::make_shared<bridge::ITwibDeviceInterface>(0, *this));
		printf("constructed TCP bridge\n");
	}

	printf("initialized Twili\n");
}

Twili::Config::Config() {
	FILE *f = fopen("/sd/twili.ini", "r");
	if(!f) {
		// write default config
		f = fopen("/sd/twili.ini", "w");
		if(!f) {
			return;
		}
		fprintf(f, "; Twili Configuration File\n");
		fprintf(f, "[twili]\n");
		fprintf(f, "service_name = %s\n", service_name.c_str());
		fprintf(f, "; paths are relative to root of sd card\n");
		fprintf(f, "hbmenu_path = %s\n", hbm_path.c_str());
		fprintf(f, "temp_directory = %s\n", temp_directory.c_str());
		fprintf(f, "\n");
		fprintf(f, "[pipes]\n");
		fprintf(f, "; 0 forces pipes to be synchronous\n");
		fprintf(f, "pipe_buffer_size_limit = 0x%lx\n", pipe_buffer_size_limit);
		fprintf(f, "\n");
		fprintf(f, "[logging]\n");
		fprintf(f, "verbosity = %d\n", logging_verbosity);
		fprintf(f, "enable_usb = %s\n", enable_usb_log ? "true" : "false");
		fprintf(f, "\n");
		fprintf(f, "[usb_bridge]\n");
		fprintf(f, "enabled = %s\n", enable_usb_bridge ? "true" : "false");
		fprintf(f, "\n");
		fprintf(f, "[tcp_bridge]\n");
		fprintf(f, "enabled = %s\n", enable_tcp_bridge ? "true" : "false");
		fprintf(f, "port = %d\n", tcp_bridge_port);
		fclose(f);
	} else {
		// load config
		INIReader reader(f);
		if(reader.ParseError() != 0) {
			state = State::Error;
			error_line = reader.ParseError();
			fclose(f);
			return;
		}

		service_name = reader.Get("twili", "service_name", service_name);
		hbm_path = reader.Get("twili", "hbmenu_path", hbm_path);
		temp_directory = reader.Get("twili", "temp_directory", temp_directory);

		pipe_buffer_size_limit = reader.GetInteger("pipes", "pipe_buffer_size_limit", pipe_buffer_size_limit);
		
		logging_verbosity = reader.GetInteger("logging", "verbosity", logging_verbosity);
		enable_usb_log = reader.GetBoolean("logging", "enable_usb", enable_usb_log);
		
		enable_usb_bridge = reader.GetBoolean("usb_bridge", "enabled", true);
		
		enable_tcp_bridge = reader.GetBoolean("tcp_bridge", "enabled", true);
		tcp_bridge_port = reader.GetInteger("tcp_bridge", "port", tcp_bridge_port);

		state = State::Loaded;
		fclose(f);
	}
}

std::shared_ptr<process::MonitoredProcess> Twili::FindMonitoredProcess(uint64_t pid) {
	auto i = std::find_if(
		monitored_processes.begin(),
		monitored_processes.end(),
		[pid](auto const &proc) {
			return proc->GetPid() == pid;
		});
	if(i == monitored_processes.end()) {
		return {};
	} else {
		return *i;
	}
}

std::shared_ptr<process::Process> Twili::FindProcess(uint64_t pid) {
	std::shared_ptr<process::Process> proc = FindMonitoredProcess(pid);
	if(!proc) {
		proc = std::make_shared<process::UnmonitoredProcess>(*this, pid);
	}
	return proc;
}

Twili::ServiceRegistration::ServiceRegistration(trn::ipc::server::IPCServer &server, std::string name, std::function<trn::Result<trn::ipc::server::Object*>(trn::ipc::server::IPCServer *server)> factory) : name(name) {
	ResultCode::AssertOk(server.CreateService(name.c_str(), factory));
}

Twili::ServiceRegistration::~ServiceRegistration() {
	ResultCode::AssertOk(sm_init());
	ResultCode::AssertOk(sm_unregister_service(name.c_str()));
	sm_finalize();
}

} // namespace twili
