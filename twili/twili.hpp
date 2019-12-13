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

#pragma once

#include<libtransistor/cpp/waiter.hpp>
#include<libtransistor/cpp/ipcserver.hpp>

#include<list>

#include "AppletTracker.hpp"
#include "process/MonitoredProcess.hpp"

#include "bridge/usb/USBBridge.hpp"
#include "bridge/tcp/TCPBridge.hpp"

#include "ipcbind/pm/IShellService.hpp"
#include "ipcbind/ldr/IDebugMonitorInterface.hpp"
#include "ipcbind/ro/IDebugMonitorInterface.hpp"
#include "ipcbind/nifm/IGeneralService.hpp"

#include "FileManager.hpp"

namespace twili {

class Twili {
 public:
	struct Config {
		Config();

		// [twili]
		std::string service_name = "twili";
		std::string hbm_path = "/hbmenu.nro";
		std::string temp_directory = "/.twili_temp";

		// [pipes]
		long pipe_buffer_size_limit = 512 * 1024;
		
		// [logging]
		int logging_verbosity = 0;
		bool enable_usb_log = true;

		// [usb_bridge]
		bool enable_usb_bridge = true;

		// [tcp_bridge]
		bool enable_tcp_bridge = true;
		int tcp_bridge_port = 15152;

		enum class State {
			Fresh, Loaded, Error
		} state = State::Fresh;
		int error_line;
	};
	
	Twili(const Config &config);

	const Config &config;
	
	bool destroy_flag = false;
	bool relaunch_flag = false;
	trn::Waiter event_waiter;
	trn::ipc::server::IPCServer server;
	
	struct ServiceRegistration {
	 public:
		ServiceRegistration(trn::ipc::server::IPCServer &server, std::string name, std::function<trn::Result<trn::ipc::server::Object*>(trn::ipc::server::IPCServer *server)> factory);
		~ServiceRegistration();
	 private:
		std::string name;
	} twili_registration;
	
	struct Services {
	 public:
		Services();

		trn::ipc::client::Object pm_dmnt;
		service::pm::IShellService pm_shell;
		service::ldr::IDebugMonitorInterface ldr_dmnt;
		service::ro::IDebugMonitorInterface ro_dmnt;
		trn::ipc::client::Object ldr_shel;
		service::nifm::IGeneralService nifm;
	} services;

	FileManager file_manager;
	AppletTracker applet_tracker;
	
	std::optional<bridge::usb::USBBridge> usb_bridge;
	std::optional<bridge::tcp::TCPBridge> tcp_bridge;
	
	std::list<std::shared_ptr<process::MonitoredProcess>> monitored_processes;
	std::shared_ptr<process::MonitoredProcess> FindMonitoredProcess(uint64_t pid);
	std::shared_ptr<process::Process> FindProcess(uint64_t pid);
	
	std::map<std::string, std::shared_ptr<TwibPipe>> named_pipes;
};

} // namespace twili
