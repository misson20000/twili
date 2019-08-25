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

#include "platform/platform.hpp"

#include<iomanip>
#include<array>

#include<string.h>
#include<inttypes.h>

#include<msgpack11.hpp>

#include "CLI/CLI.hpp"

#include "common/Logger.hpp"
#include "common/ResultError.hpp"
#include "common/config.hpp"

#include "platform/InputPump.hpp"

#include "Protocol.hpp"
#include "interfaces/ITwibMetaInterface.hpp"
#include "interfaces/ITwibDeviceInterface.hpp"

#if TWIB_GDB_ENABLED == 1
#include "GdbStub.hpp"
#endif

#if TWIB_TCP_FRONTEND_ENABLED == 1 || TWIB_UNIX_FRONTEND_ENABLED == 1
#include "SocketClient.hpp"
#endif
#if TWIB_NAMED_PIPE_FRONTEND_ENABLED == 1
#include "NamedPipeClient.hpp"
#endif

#include "util.hpp"
#include "err.hpp"

namespace twili {
namespace twib {
namespace tool {

template<size_t N>
void PrintTable(std::vector<std::array<std::string, N>> rows) {
	std::array<int, N> lengths = {0};
	for(auto r : rows) {
		for(size_t i = 0; i < N; i++) {
			if(r[i].size() > lengths[i]) {
				lengths[i] = r[i].size();
			}
		}
	}
	for(auto r : rows) {
		for(size_t i = 0; i < N; i++) {
			printf("%-*s%s", lengths[i], r[i].c_str(), (i + 1 == N) ? "\n" : " | ");
		}
	}
}

template<typename T>
std::string ToHex(T num, int width, bool prefix) {
	std::stringstream stream;
	stream << (prefix ? "0x" : "") << std::setfill('0') << std::setw(width) << std::hex << num;
	return stream.str();
}

template<typename T>
std::string ToHex(T num, bool prefix) {
	std::stringstream stream;
	stream << (prefix ? "0x" : "") << std::hex << num;
	return stream.str();
}

void ListDevices(ITwibMetaInterface &iface) {
	std::vector<std::array<std::string, 4>> rows;
	rows.push_back({"Device ID", "Nickname", "Firmware Version", "Bridge Type"});
	auto devices = iface.ListDevices();
	for(msgpack11::MsgPack device : devices) {
		uint32_t device_id = device["device_id"].uint32_value();
		std::string bridge_type = device["bridge_type"].string_value();
		msgpack11::MsgPack ident = device["identification"];
		auto nickname = ident["device_nickname"].string_value();
		auto version = ident["firmware_version"].binary_items();
		std::string fw_version((char*) version.data() + 0x68);
		
		rows.push_back({ToHex(device_id, 8, false), nickname, fw_version, bridge_type});
	}
	PrintTable(rows);
}

void ListProcesses(ITwibDeviceInterface &iface) {
	std::vector<std::array<std::string, 5>> rows;
	rows.push_back({"Process ID", "Result", "Title ID", "Process Name", "MMU Flags"});
	auto processes = iface.ListProcesses();
	for(auto p : processes) {
		rows.push_back({
			ToHex(p.process_id, true),
			ToHex(p.result, true),
			ToHex(p.title_id, true),
			std::string(p.process_name, 12),
			ToHex(p.mmu_flags, true)});
	}
	PrintTable(rows);
}

std::unique_ptr<client::Client> connect_tcp(uint16_t port);
std::unique_ptr<client::Client> connect_unix(std::string path);
std::unique_ptr<client::Client> connect_named_pipe(std::string path);

} // namespace tool
} // namespace twib
} // namespace twili

void show(msgpack11::MsgPack const& blob);

using namespace twili;
using namespace twili::twib;

class FSCommands {
 public:
	FSCommands(CLI::App &app, const char *cmdname, const char *desc, const char *fsname) :
		app(app),
		cmdname(cmdname),
		fsname(fsname) {
		subcommand = app.add_subcommand(cmdname, desc);

		pull = subcommand->add_subcommand("pull", "Pulls files from device filesystem to host filesystem");
		pull->add_option("from", pull_from, "Path(s) to pull from (on device)")->expected(-1);
		pull->add_option("to", pull_to, "Path to write to (on host)");

		push = subcommand->add_subcommand("push", "Pushes files from host filesystem to device filesystem");
		push->add_option("from", push_from, "Path(s) to read from (on host)")->expected(-1);
		push->add_option("to", push_to, "Path to write to (on device)");

		ls = subcommand->add_subcommand("ls", "Lists files on device filesystem");
		ls->add_flag("-l", ls_details, "Show more details");
		ls->add_option("path", ls_path, "Directory to list files in");

		rm = subcommand->add_subcommand("rm", "Removes a file or directory");
		rm->add_flag("-r", rm_recursive, "Delete recursively");
		rm->add_option("path", rm_path, "File or directory to remove")->required();

		mkdir = subcommand->add_subcommand("mkdir", "Creates a directory");
		mkdir->add_option("path", mkdir_path, "Directory to create")->required();

		mv = subcommand->add_subcommand("mv", "Rename a file or directory");
		mv->add_option("src", mv_src, "Source path")->required();
		mv->add_option("dst", mv_dst, "Destination path")->required();
		
		subcommand->require_subcommand(1);
	}

	int Run(tool::ITwibDeviceInterface &itdi) {
		if(pull->parsed()) {
			return DoPull(itdi);
		}
		if(push->parsed()) {
			return DoPush(itdi);
		}
		if(ls->parsed()) {
			return DoLs(itdi);
		}
		if(rm->parsed()) {
			return DoRm(itdi);
		}
		if(mkdir->parsed()) {
			return DoMkdir(itdi);
		}
		if(mv->parsed()) {
			return DoMv(itdi);
		}
		return 0;
	}

	int DoPull(tool::ITwibDeviceInterface &itdi) {
		struct stat target_stat;
		bool is_target_directory = false;

		// stupid hack for stupid command line parser
		if(pull_from.size() > 1) {
			pull_to = pull_from.back();
			pull_from.pop_back();
		}
		
		if(pull_to != "-") {
			std::optional<platform::fs::Stat> target_stat = platform::fs::StatFile(pull_to.c_str());
			is_target_directory = target_stat && target_stat->is_directory;

			if(pull_from.size() > 1 && !is_target_directory) {
				LogMessage(Error, "target '%s' is not a directory", pull_to.c_str());
				return 1;
			}

			if(is_target_directory) {
				if(pull_to.back() != '/') {
					pull_to.push_back('/');
				}
			}
		}

		tool::ITwibFilesystemAccessor itfsa = itdi.OpenFilesystemAccessor(fsname);
		
		for(std::string &src : pull_from) {
			tool::ITwibFileAccessor itfa = itfsa.OpenFile(1, "/" + src);
			
			std::string dst_path;
			platform::File dst;
			if(pull_to == "-") {
				dst_path = "<stdout>";
				dst = platform::File::BorrowStdout();
			} else {
				if(is_target_directory) {
					dst_path = pull_to + src;
				} else {
					dst_path = pull_to;
				}
				dst = platform::File::OpenForClobberingWrite(dst_path.c_str());
			}

			size_t total_size = itfa.GetSize();
			size_t offset = 0;
			while(offset < total_size) {
				std::vector<uint8_t> data = itfa.Read(offset, total_size - offset);
				if(data.size() == 0 || dst.Write(data.data(), data.size()) < data.size()) {
					LogMessage(Error, "hit EoF/IO error unexpectedly?");
					return 1;
				}
				offset+= data.size();
			}

			if(pull_to != "-") {
				fprintf(stderr, "%s -> %s\n", src.c_str(), dst_path.c_str());
			}
		}
		return 0;
	}

	int DoPush(tool::ITwibDeviceInterface &itdi) {
		bool is_target_directory = false;

		// stupid hack for stupid command line parser
		if(push_from.size() > 1) {
			push_to = push_from.back();
			push_from.pop_back();
		}

		if(push_to.front() != '/') {
			push_to.insert(push_to.begin(), '/');
		}

		tool::ITwibFilesystemAccessor itfsa = itdi.OpenFilesystemAccessor(fsname);

		LogMessage(Debug, "checking if is file");
		std::optional<bool> is_file_result = itfsa.IsFile(push_to);
		LogMessage(Debug, "checked if is file");
		is_target_directory = is_file_result && !(*is_file_result);

		if(push_from.size() > 1 && !is_target_directory) {
			LogMessage(Error, "target '%s' is not a directory", push_to.c_str());
			return 1;
		}
		
		if(is_target_directory) {
			if(push_to.back() != '/') {
				push_to.push_back('/');
			}
		}
		
		for(std::string &src_path : push_from) {
			std::string dst_path;
			platform::File src = platform::File::OpenForRead(src_path.c_str());
			
			if(is_target_directory) {
				dst_path = push_to + platform::fs::BaseName(src_path.c_str()); // lmao super dangerous don't ever do this
			} else {
				dst_path = push_to;
			}

			size_t total_size = src.GetSize();

			LogMessage(Debug, "creating %s", dst_path.c_str());
			itfsa.CreateFile(0, total_size, dst_path);
			LogMessage(Debug, "opening %s", dst_path.c_str());
			tool::ITwibFileAccessor itfa = itfsa.OpenFile(6, dst_path);
			LogMessage(Debug, "setting size");
			itfa.SetSize(total_size);

			size_t offset = 0;
			std::vector<uint8_t> data;
			while(offset < total_size) {
				data.resize(std::min(total_size - offset, (size_t) 0x10000));
				size_t r;
				if((r = src.Read(data.data(), data.size())) < data.size()) {
					LogMessage(Error, "hit EoF unexpectedly? expected 0x%lx, got 0x%lx", data.size(), r);
					return 1;
				}
				
				itfa.Write(offset, data);
				offset+= data.size();
			}

			fprintf(stderr, "%s -> %s\n", src_path.c_str(), dst_path.c_str());
		}

		return 0;
	}

	int DoLs(tool::ITwibDeviceInterface &itdi) {
		tool::ITwibFilesystemAccessor itfsa = itdi.OpenFilesystemAccessor(fsname);
		tool::ITwibDirectoryAccessor itda = itfsa.OpenDirectory(ls_path);

		uint64_t read = 0;
		std::vector<tool::ITwibDirectoryAccessor::DirectoryEntry> entries;
		entries.resize(itda.GetEntryCount());

		while(read < entries.size()) {
			auto batch = itda.Read();
			if(read + batch.size() > entries.size()) {
				LogMessage(Error, "too many entries");
				return 1;
			}
			std::copy(batch.begin(), batch.end(), entries.begin() + read);
			read+= batch.size();
		}

		std::sort(entries.begin(), entries.end(), [](auto &a, auto &b) -> bool { return strcmp(a.path, b.path) < 1; });
		
		for(auto &e : entries) {
			if(ls_details) {
				printf("%s%s %9d  %s\n", e.entry_type == 0 ? "d" : "-", e.attributes & 1 ? "a" : "-", e.file_size, e.path);
			} else {
				printf("%s\n", e.path);
			}
		}

		return 0;
	}

	int DoRm(tool::ITwibDeviceInterface &itdi) {
		tool::ITwibFilesystemAccessor itfsa = itdi.OpenFilesystemAccessor(fsname);
		std::optional<bool> is_file_result = itfsa.IsFile(rm_path);
		if(!is_file_result) {
			fprintf(stderr, "'%s': No such file or directory\n", rm_path.c_str());
			return 1;
		}

		if(*is_file_result) {
			itfsa.DeleteFile(rm_path);
		} else {
			if(rm_recursive) {
				itfsa.DeleteDirectoryRecursively(rm_path);
			} else {
				itfsa.DeleteDirectory(rm_path);
			}
		}
		return 0;
	}

	int DoMkdir(tool::ITwibDeviceInterface &itdi) {
		tool::ITwibFilesystemAccessor itfsa = itdi.OpenFilesystemAccessor(fsname);
		if(!itfsa.CreateDirectory(mkdir_path)) {
			fprintf(stderr, "'%s': File exists\n", mkdir_path.c_str());
			return 1;
		}
		return 0;
	}

	int DoMv(tool::ITwibDeviceInterface &itdi) {
		tool::ITwibFilesystemAccessor itfsa = itdi.OpenFilesystemAccessor(fsname);
		std::optional<bool> is_src_file = itfsa.IsFile(mv_src);
		if(!is_src_file) {
			fprintf(stderr, "'%s': No such file or directory\n", mv_src.c_str());
			return 1;
		}
		
		std::optional<bool> is_dst_file = itfsa.IsFile(mv_dst);
		if(is_dst_file && !*is_dst_file) { // destination is directory and exists
			if(mv_dst.back() != '/') {
				mv_dst.push_back('/');
			}
			mv_dst+= platform::fs::BaseName(mv_src.c_str()); // append source filename
		}

		if(*is_src_file) {
			itfsa.RenameFile(mv_src, mv_dst);
		} else {
			itfsa.RenameDirectory(mv_src, mv_dst);
		}
		
		return 0;
	}

	CLI::App &app;
	CLI::App *subcommand;
	
 private:
	CLI::App *pull;
	std::vector<std::string> pull_from;
	std::string pull_to = ".";
	
	CLI::App *push;
	std::vector<std::string> push_from;
	std::string push_to = "/";

	CLI::App *ls;
	bool ls_details;
	std::string ls_path = "/";
	
	CLI::App *rm;
	bool rm_recursive;
	std::string rm_path;
	
	CLI::App *mkdir;
	std::string mkdir_path;

	CLI::App *mv;
	std::string mv_src;
	std::string mv_dst;
	
	const char *cmdname;
	const char *fsname;
};

int main(int argc, char *argv[]) {
#ifdef _WIN32
	WSADATA wsaData;
	int err;
	err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0) {
		printf("WSAStartup failed with error: %d\n", err);
		return 1;
	}
#endif

	CLI::App app {"Twili debug monitor client"};

	std::string device_id_str;
	app.add_option("-d,--device", device_id_str, "Use a specific device")
		->type_name("DeviceId")
		->envname("TWIB_DEVICE");

	bool is_verbose;
	app.add_flag("-v,--verbose", is_verbose, "Enable debug logging");

	std::string frontend;
	std::string unix_frontend_path = TWIB_UNIX_FRONTEND_DEFAULT_PATH;
	uint16_t tcp_frontend_port = TWIB_TCP_FRONTEND_DEFAULT_PORT;
	std::string named_pipe_frontend_path = TWIB_NAMED_PIPE_FRONTEND_DEFAULT_NAME;
	
#if TWIB_NAMED_PIPE_FRONTEND_ENABLED == 1
	frontend = "named_pipe";
#elif TWIB_UNIX_FRONTEND_ENABLED == 1
	frontend = "unix";
#else
	frontend = "tcp";
#endif
	
	app.add_set("-f,--frontend", frontend, {
#if TWIB_NAMED_PIPE_FRONTEND_ENABLED == 1
			"named_pipe",
#endif
#if TWIB_UNIX_FRONTEND_ENABLED == 1
			"unix",
#endif
#if TWIB_TCP_FRONTEND_ENABLED == 1
			"tcp",
#endif
		})->envname("TWIB_FRONTEND");

#if TWIB_UNIX_FRONTEND_ENABLED == 1
	app.add_option(
		"-P,--unix-path", unix_frontend_path,
		"Path to the twibd UNIX socket")
		->envname("TWIB_UNIX_FRONTEND_PATH");
#endif

#if TWIB_TCP_FRONTEND_ENABLED == 1
	app.add_option(
		"-p,--tcp-port", tcp_frontend_port,
		"Port for the twibd TCP socket")
		->envname("TWIB_TCP_FRONTEND_PORT");
#endif

#if TWIB_NAMED_PIPE_FRONTEND_ENABLED == 1
	app.add_option(
		"-n,--pipe-name", named_pipe_frontend_path,
		"Named for the twibd pipe")
		->envname("TWIB_NAMED_PIPE_FRONTEND_NAME");
#endif
	
	CLI::App *ld = app.add_subcommand("list-devices", "List devices");
	
	CLI::App *cmd_connect_tcp = app.add_subcommand("connect-tcp", "Connect to a device over TCP");
	std::string connect_tcp_hostname;
	std::string connect_tcp_port = "15152";
	cmd_connect_tcp->add_option("hostname", connect_tcp_hostname, "Hostname to connect to")->required();
	cmd_connect_tcp->add_option("port", connect_tcp_port, "Port to connect to");
	
	CLI::App *run = app.add_subcommand("run", "Run an executable");
	std::string run_file;
	bool run_applet = false;;
	bool run_suspend = false;
	bool run_quiet = false;
	run->add_flag("-a,--applet", run_applet, "Run as an applet");
	run->add_flag("-d,--debug-suspend", run_suspend, "Suspends for debug");
	run->add_flag("-q,--quiet", run_quiet, "Suppress any output except from the program being run");
	run->add_option("file", run_file, "Executable to run")->check(CLI::ExistingFile)->required();
	
	CLI::App *reboot = app.add_subcommand("reboot", "Reboot the device");

	CLI::App *coredump = app.add_subcommand("coredump", "Make a coredump of a crashed process");
	std::string core_file;
	uint64_t core_process_id;
	coredump->add_option("file", core_file, "File to dump core to")->required();
	coredump->add_option("pid", core_process_id, "Process ID")->required();
	
	CLI::App *terminate = app.add_subcommand("terminate", "Terminate a process on the device");
	uint64_t terminate_process_id;
	terminate->add_option("pid", terminate_process_id, "Process ID")->required();
	
	CLI::App *ps = app.add_subcommand("ps", "List processes on the device");

	CLI::App *identify = app.add_subcommand("identify", "Identify the device");

	CLI::App *list_named_pipes = app.add_subcommand("list-named-pipes", "List named pipes on the device");

	CLI::App *open_named_pipe = app.add_subcommand("open-named-pipe", "Open a named pipe on the device");
	std::string open_named_pipe_name;
	open_named_pipe->add_option("name", open_named_pipe_name, "Name of pipe to open")->required();

	CLI::App *get_memory_info = app.add_subcommand("get-memory-info", "Gets memory usage information from the device");

	CLI::App *print_debug_info = app.add_subcommand("debug", "Prints debug info");

#if TWIB_GDB_ENABLED == 1
	CLI::App *gdb = app.add_subcommand("gdb", "Opens an enhanced GDB stub for the device");
#endif

	CLI::App *launch = app.add_subcommand("launch", "Launches an installed title");
	std::string launch_title_id;
	std::string launch_storage;
	uint32_t launch_flags = 0;
	launch->add_option("title-id", launch_title_id, "Title ID to launch")->required();
	launch->add_set_ignore_case("storage", launch_storage, {"host", "gamecard", "gc", "nand-system", "system", "nand-user", "user", "sdcard", "sd"}, "Storage for title")->required();
	launch->add_option("launch-flags", launch_flags, "Flags for launch");

	FSCommands sd_commands(app, "sd", "Perform operations on target SD card", "sd");
	FSCommands nand_user_commands(app, "nu", "Perform operations on target NAND user filesystem", "nand_user");
	FSCommands nand_system_commands(app, "ns", "Perform operations on target NAND system filesystem", "nand_system");
	
	app.require_subcommand(1);
	
	try {
		app.parse(argc, argv);
	} catch(const CLI::ParseError &e) {
		return app.exit(e);
	}

	log::init_color();
	if(is_verbose) {
#if TWIB_GDB_ENABLED == 1
		if(gdb->parsed()) {
			// for gdb stub, all logging should go to stderr
			log::add_log(std::make_shared<log::PrettyFileLogger>(stderr, log::Level::Debug, log::Level::Error));
#else
		if(false) {
#endif
		} else {
			log::add_log(std::make_shared<log::PrettyFileLogger>(stdout, log::Level::Debug, log::Level::Error));
		}
	}
	log::add_log(std::make_shared<log::PrettyFileLogger>(stderr, log::Level::Error));
	
	LogMessage(Message, "starting twib");

	std::unique_ptr<tool::client::Client> client;
	if(TWIB_UNIX_FRONTEND_ENABLED && frontend == "unix") {
		client = tool::connect_unix(unix_frontend_path);
	} else if(TWIB_TCP_FRONTEND_ENABLED && frontend == "tcp") {
		client = tool::connect_tcp(tcp_frontend_port);
	} else if(TWIB_NAMED_PIPE_FRONTEND_ENABLED && frontend == "named_pipe") {
		client = tool::connect_named_pipe(named_pipe_frontend_path);
	} else {
		LogMessage(Fatal, "unrecognized frontend: %s", frontend.c_str());
		return 1;
	}
	if(!client) {
		return 1;
	}
	
	tool::ITwibMetaInterface itmi(tool::RemoteObject(*client, 0, 0));
	
	if(ld->parsed()) {
		ListDevices(itmi);
		return 0;
	}

	if(cmd_connect_tcp->parsed()) {
		printf("%s\n", itmi.ConnectTcp(connect_tcp_hostname, connect_tcp_port).c_str());
		return 0;
	}

	uint32_t device_id;
	if(device_id_str.size() > 0) {
		device_id = std::stoul(device_id_str, NULL, 16);
	} else {
		std::vector<msgpack11::MsgPack> devices = itmi.ListDevices();
		if(devices.size() == 0) {
			LogMessage(Fatal, "No devices were detected.");
			return 1;
		}
		if(devices.size() > 1) {
			LogMessage(Fatal, "Multiple devices were detected. Please use -d to specify which one you mean.");
			return 1;
		}
		device_id = devices[0]["device_id"].uint32_value();
	}
	tool::ITwibDeviceInterface itdi(std::make_shared<tool::RemoteObject>(*client, device_id, 0));
	
	if(run->parsed()) {
		auto code_opt = util::ReadFile(run_file.c_str());
		if(!code_opt) {
			LogMessage(Fatal, "could not read file");
			return 1;
		}
		
		tool::ITwibProcessMonitor mon = itdi.CreateMonitoredProcess(run_applet ? "applet" : "managed");
		mon.AppendCode(*code_opt);
		uint64_t pid = run_suspend ? mon.LaunchSuspended() : mon.Launch();
		if(!run_quiet) {
			printf("PID: 0x%" PRIx64"\n", pid);
		}
		auto pump_output =
			[](tool::ITwibPipeReader reader, FILE *stream) {
				try {
					while(true) {
						std::vector<uint8_t> str = reader.ReadSync();
						size_t r = fwrite(str.data(), sizeof(str[0]), str.size(), stream);
						if(r < str.size() && str.size() > 0) {
							throw std::system_error(errno, std::generic_category());
						}
						fflush(stream);
					}
				} catch(ResultError &e) {
					LogMessage(Debug, "output pump got 0x%x", e.code);
					if(e.code == TWILI_ERR_EOF) {
						LogMessage(Debug, "  EoF");
						return;
					} else {
						return; // there really isn't much we can do with this
					}
				}
			};
		std::thread stdout_pump(pump_output, mon.OpenStdout(), stdout);
		std::thread stderr_pump(pump_output, mon.OpenStderr(), stderr);

		class Logic : public platform::EventLoop::Logic {
		 public:
			Logic(std::function<void(platform::EventLoop&)> f) : f(f) {
			}
			virtual void Prepare(platform::EventLoop &loop) override {
				f(loop);
			};
		 private:
			std::function<void(platform::EventLoop&)> f;
		};

		tool::ITwibPipeWriter r = mon.OpenStdin();
		platform::InputPump input_pump(4096,
			[&r](std::vector<uint8_t> &data) {
				r.WriteSync(data);
			}, [&r]() {
				r.Close();
			});
		
		Logic logic(
			[&](platform::EventLoop &l) {
				l.Clear();
				l.AddMember(input_pump);
			});
		platform::EventLoop stdin_loop(logic);
		stdin_loop.Begin();
		
		stdout_pump.join();
		stderr_pump.join();
		LogMessage(Debug, "output pump threads exited");
		try {
			uint32_t state;
			while((state = mon.WaitStateChange()) != 6) {
				LogMessage(Debug, "  state %d change...", state);
			}
		} catch(ResultError &e) {
			LogMessage(Error, "got 0x%x waiting for process exit", e.code);
		}
		LogMessage(Debug, "  process exited");
		stdin_loop.Destroy();
		return 0;
	}

	if(reboot->parsed()) {
		itdi.Reboot();
		return 0;
	}

	if(coredump->parsed()) {
		FILE *f = fopen(core_file.c_str(), "wb");
		if(!f) {
			LogMessage(Fatal, "could not open '%s': %s", core_file.c_str(), strerror(errno));
			return 1;
		}
		std::vector<uint8_t> core = itdi.CoreDump(core_process_id);
		size_t written = 0;
		while(written < core.size()) {
			ssize_t r = fwrite(core.data() + written, 1, core.size() - written, f);
			if(r <= 0 || ferror(f)) {
				LogMessage(Fatal, "write error on '%s'");
			} else {
				written+= r;
			}
		}
		fclose(f);
	}
	
	if(terminate->parsed()) {
		itdi.Terminate(terminate_process_id);
		return 0;
	}

	if(ps->parsed()) {
		ListProcesses(itdi);
		return 0;
	}

	if(identify->parsed()) {
		show(itdi.Identify());
		return 0;
	}

	if(list_named_pipes->parsed()) {
		for(auto n : itdi.ListNamedPipes()) {
			printf("%s\n", n.c_str());
		}
		return 0;
	}

	if(open_named_pipe->parsed()) {
		auto reader = itdi.OpenNamedPipe(open_named_pipe_name);
		try {
			while(true) {
				std::vector<uint8_t> str = reader.ReadSync();
				std::cout << std::string(str.begin(), str.end());
			}
		} catch(ResultError &e) {
			if(e.code == TWILI_ERR_EOF) {
				return 0;
			} else {
				throw e;
			}
		}
		return 0;
	}

	if(get_memory_info->parsed()) {
		msgpack11::MsgPack meminfo = itdi.GetMemoryInfo();
		uint64_t total_memory_available = meminfo["total_memory_available"].uint64_value();
		uint64_t total_memory_usage     = meminfo["total_memory_usage"    ].uint64_value();
		const size_t one_mib = 1024 * 1024;
		printf(
			"Twili Memory: %" PRIu64" MiB / %" PRIu64" MiB (%" PRIu64"%%)\n",
			total_memory_usage / one_mib,
			total_memory_available / one_mib,
			total_memory_usage * 100 / total_memory_available);

		std::vector<const char*> category_labels = {"System", "Application", "Applet"};
		for(auto &cat_info : meminfo["limits"].array_items()) {
			printf(
				"%s Category Limit: %" PRIu64" MiB / %" PRIu64" MiB (%" PRIu64"%%)\n",
				category_labels[cat_info["category"].int_value()],
				cat_info["current_value"].uint64_value() / one_mib,
				cat_info["limit_value"].uint64_value() / one_mib,
				cat_info["current_value"].uint64_value() * 100 / cat_info["limit_value"].uint64_value());
		}
		return 0;
	}

	if(print_debug_info->parsed()) {
		itdi.PrintDebugInfo();
		return 0;
	}

	if(launch->parsed()) {
		uint64_t storage_id = 0;
		if(launch_storage == "none") {
			storage_id = 0;
		} else if(launch_storage == "host") {
			storage_id = 1;
		} else if(launch_storage == "gamecard" || launch_storage == "gc") {
			storage_id = 2;
		} else if(launch_storage == "nand-system" || launch_storage == "system") {
			storage_id = 3;
		} else if(launch_storage == "nand-user" || launch_storage == "user") {
			storage_id = 4;
		} else if(launch_storage == "sdcard" || launch_storage == "sd") {
			storage_id = 5;
		} else {
			LogMessage(Error, "unrecognized storage: %s\n", launch_storage.c_str());
		}

		uint64_t title_id = std::stoull(launch_title_id, nullptr, 16);

		printf("0x%" PRIx64"\n", itdi.LaunchUnmonitoredProcess(title_id, storage_id, launch_flags));
	}

#if TWIB_GDB_ENABLED == 1
	if(gdb->parsed()) {
		tool::gdb::GdbStub stub(itdi);
		stub.Run();
		return 0;
	}
#endif

	if(sd_commands.subcommand->parsed()) {
		return sd_commands.Run(itdi);
	}

	if(nand_user_commands.subcommand->parsed()) {
		return nand_user_commands.Run(itdi);
	}

	if(nand_system_commands.subcommand->parsed()) {
		return nand_system_commands.Run(itdi);
	}

	return 0;
}

namespace twili {
namespace twib {
namespace tool {

std::unique_ptr<client::Client> connect_tcp(uint16_t port) {
#if TWIB_TCP_FRONTEND_ENABLED == 0
	LogMessage(Fatal, "TCP socket not supported");
	return std::unique_ptr<client::Client>();
#else
	platform::Socket socket(AF_INET6, SOCK_STREAM, 0);

	struct sockaddr_in6 addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_addr = in6addr_loopback;
	addr.sin6_port = htons(port);

	socket.Connect((struct sockaddr *) &addr, sizeof(addr));
	LogMessage(Info, "connected to twibd: %d", socket.fd);
	
	return std::make_unique<client::SocketClient>(std::move(socket));
#endif
}

std::unique_ptr<client::Client> connect_unix(std::string path) {
#if TWIB_UNIX_FRONTEND_ENABLED == 0
	LogMessage(Fatal, "UNIX domain socket not supported");
	return std::unique_ptr<client::Client>();
#else
	platform::Socket socket(AF_UNIX, SOCK_STREAM, 0);

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

	socket.Connect((struct sockaddr *) &addr, sizeof(addr));
	LogMessage(Info, "connected to twibd: %d", socket.fd);
	
	return std::make_unique<client::SocketClient>(std::move(socket));
#endif
}

std::unique_ptr<client::Client> connect_named_pipe(std::string path) {
#if TWIB_NAMED_PIPE_FRONTEND_ENABLED == 0
	LogMessage(Fatal, "Named pipe not supported");
	return std::unique_ptr<client::Client>();
#else
	twili::platform::windows::Pipe pipe; 
	LogMessage(Debug, "connecting to %s...", path.c_str());
	while(1) {
		pipe = platform::windows::Pipe::OpenNamed(path.c_str());
		if(pipe.handle != INVALID_HANDLE_VALUE) {
			break;
		}

		if(GetLastError() != ERROR_PIPE_BUSY) {
			LogMessage(Fatal, "Could not open pipe. GLE=%d", GetLastError());
			return std::unique_ptr<client::Client>();
		}

		LogMessage(Info, "got ERROR_PIPE_BUSY, waiting...");
		if(!WaitNamedPipe(path.c_str(), 20000)) {
			LogMessage(Fatal, "Could not open pipe: 20 second wait timed out");
			return std::unique_ptr<client::Client>();
		}
	}
	/*DWORD mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
	if(!SetNamedPipeHandleState(pipe.handle, &mode, nullptr, nullptr)) {
		LogMessage(Fatal, "Failed to set named pipe handle state. GLE=%d", GetLastError());
		return std::unique_ptr<client::Client>();
	}*/
	return std::make_unique<client::NamedPipeClient>(std::move(pipe));
#endif
}

} // namespace tool
} // namespace twib
} // namespace twili
