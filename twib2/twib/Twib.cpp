#include "Twib.hpp"

#include<random>
#include<iomanip>

#include<sys/socket.h>
#include<sys/un.h>
#include<string.h>
#include<unistd.h>
#include<poll.h>
#include<msgpack11.hpp>

#include "CLI/CLI.hpp"

#include "Logger.hpp"
#include "config.hpp"
#include "Protocol.hpp"

namespace twili {
namespace twib {

Twib::Twib() {
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(fd < 0) {
		log(FATAL, "failed to create UNIX domain socket: %s", strerror(errno));
		exit(1);
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, twibd::frontend::Twibd_UNIX_SOCKET_PATH, sizeof(addr.sun_path)-1);

	if(connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		log(FATAL, "failed to connect to twibd: %s", strerror(errno));
		close(fd);
		exit(1);
	}
	log(INFO, "connected to twibd");

	if(pipe(event_thread_notification_pipe) < 0) {
		log(FATAL, "failed to create pipe for event thread notifications: %s", strerror(errno));
		close(fd);
		exit(1);
	}
	
	std::thread event_thread(&Twib::event_thread_func, this);
	this->event_thread = std::move(event_thread);
}

Twib::~Twib() {
	event_thread_destroy = true;
	NotifyEventThread();
	event_thread.join();

	close(fd);
}

void Twib::event_thread_func() {
	while(!event_thread_destroy) {
		log(DEBUG, "event thread loop");

		struct pollfd pollfds[2];
		pollfds[0] = {.fd = event_thread_notification_pipe[0], .events = POLLIN};
		pollfds[1] = {.fd = fd, .events = POLLIN};
		if(out_buffer.size() > 0) {
			pollfds[1].events|= POLLOUT;
		}

		if(poll(pollfds, sizeof(pollfds)/sizeof(pollfds[0]), -1) < 0) {
			log(FATAL, "failed to poll file descriptors: %s", strerror(errno));
			exit(1);
		}

		// check poll flags on event notification pipe
		if(pollfds[0].revents & POLLIN) {
			char buf[64];
			ssize_t r = read(event_thread_notification_pipe[0], buf, sizeof(buf));
			if(r < 0) {
				log(FATAL, "failed to read from event thread notification pipe: %s", strerror(errno));
				exit(1);
			}
			log(DEBUG, "event thread notified: '%.*s'", r, buf);
		}

		if(pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			log(FATAL, "event thread notification pipe has bad revents flags: 0x%x", pollfds[0].revents);
			exit(1);
		}

		// check poll flags on twibd socket
		if(pollfds[1].revents & POLLOUT) {
			PumpOutput();
		}
		if(pollfds[1].revents & POLLIN) {
			PumpInput();
		}
		if(pollfds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			log(FATAL, "twibd hung up");
			exit(1);
		}
		ProcessResponses();
	}
}

void Twib::NotifyEventThread() {
	char buf[] = ".";
	if(write(event_thread_notification_pipe[1], buf, sizeof(buf)) != sizeof(buf)) {
		log(FATAL, "failed to write to event thread notification pipe: %s", strerror(errno));
		exit(1);
	}
}

void Twib::PumpOutput() {
	std::lock_guard<std::mutex> lock(out_buffer_mutex);
	if(out_buffer.size() > 0) {
		ssize_t r = send(fd, out_buffer.data(), out_buffer.size(), 0);
		if(r < 0) {
			log(FATAL, "failed to write to twibd: %s", strerror(errno));
			exit(1);
		}
		if(r > 0) {
			// move everything that we didn't send to the start of the buffer
			std::move(out_buffer.begin() + r, out_buffer.end(), out_buffer.begin());
			out_buffer.resize(out_buffer.size() - r);
		}
	}
}

void Twib::PumpInput() {
	log(DEBUG, "pumping input");
	
	size_t old_size = in_buffer.size();

	// make some space to read into
	if(in_buffer.size() < in_buffer_size_hint) {
		in_buffer.resize(in_buffer_size_hint);
	} else {
		in_buffer.resize(old_size + 4096);
	}

	// read into the space we just made
	ssize_t r = recv(fd, in_buffer.data() + old_size, in_buffer.size() - old_size, 0);
	if(r < 0) {
		log(FATAL, "failed to receive from twibd: %s", strerror(errno));
		exit(1);
	} else {
		// set the size of the buffer to reflect what we read
		in_buffer.resize(old_size + r);
	}
}

void Twib::ProcessResponses() {
	while(in_buffer.size() > 0) {
		if(in_buffer.size() < sizeof(MessageHeader)) {
			in_buffer_size_hint = sizeof(MessageHeader);
			return;
		}
		MessageHeader &mh = *(MessageHeader*) in_buffer.data();
		size_t total_message_size = sizeof(MessageHeader) + mh.payload_size;
		if(in_buffer.size() < total_message_size) {
			in_buffer_size_hint = total_message_size;
			return;
		}
		
		std::vector<uint8_t> payload(
			in_buffer.begin() + sizeof(MessageHeader),
			in_buffer.begin() + total_message_size);
		
		std::promise<Response> promise;
		{
			std::lock_guard<std::mutex> lock(response_map_mutex);
			auto it = response_map.find(mh.tag);
			if(it == response_map.end()) {
				log(WARN, "dropping response for unknown tag 0x%x", mh.tag);
				continue;
			}
			promise.swap(it->second);
			response_map.erase(it);
		}
		promise.set_value(Response(mh.device_id, mh.object_id, mh.result_code, mh.tag, payload));
		
		// move everything past the end of the message we just processed to the start of the buffer
		std::move(in_buffer.begin() + total_message_size, in_buffer.end(), in_buffer.begin());
		in_buffer.resize(in_buffer.size() - total_message_size);
	}
}

std::future<Response> Twib::SendRequest(Request rq) {
	std::future<Response> future;
	{
		std::lock_guard<std::mutex> lock(response_map_mutex);
		std::mt19937 rng;
		
		uint32_t tag = rng();
		rq.tag = tag;
		
		std::promise<Response> promise;
		future = promise.get_future();
		response_map[tag] = std::move(promise);
	}
	{
		std::lock_guard<std::mutex> lock(twibd_mutex);
		MessageHeader mh;
		mh.device_id = rq.device_id;
		mh.object_id = rq.object_id;
		mh.command_id = rq.command_id;
		mh.tag = rq.tag;
		mh.payload_size = rq.payload.size();

		ssize_t r = send(fd, &mh, sizeof(mh), 0);
		if(r < sizeof(mh)) {
			log(FATAL, "I/O error when sending request header");
			exit(1);
		}

		for(size_t sent = 0; sent < rq.payload.size(); sent+= r) {
			r = send(fd, rq.payload.data() + sent, rq.payload.size() - sent, 0);
			if(r <= 0) {
				log(FATAL, "I/O error when sending request payload");
				exit(1);
			}
		}

		log(DEBUG, "sent request");
	}
	
	return future;
}

} // namespace twib
} // namespace twili

int main(int argc, char *argv[]) {
	CLI::App app{"Twili debug monitor client"};

	std::string device_id_str;
	app.add_option("-d,--device", device_id_str, "Use a specific device")->type_name("DeviceId");

	CLI::App *ld = app.add_subcommand("list-devices", "List devices");
	
	CLI::App *run = app.add_subcommand("run", "Run an executable");
	std::string run_file;
	run->add_option("file", run_file, "Executable to run")->check(CLI::ExistingFile)->required();
	
	CLI::App *reboot = app.add_subcommand("reboot", "Reboot the device");
	
	CLI::App *terminate = app.add_subcommand("terminate", "Terminate a process on the device");
	uint64_t terminate_process_id;
	terminate->add_option("pid", terminate_process_id, "Process ID")->required();
	
	CLI::App *ps = app.add_subcommand("ps", "List processes on the device");
	app.require_subcommand(1);
	
	try {
		app.parse(argc, argv);
	} catch(const CLI::ParseError &e) {
		return app.exit(e);
	}

	add_log(new twili::log::PrettyFileLogger(stdout, twili::log::Level::DEBUG, twili::log::Level::ERROR));
	add_log(new twili::log::PrettyFileLogger(stderr, twili::log::Level::ERROR));

	log(MSG, "starting twib");
	
	twili::twib::Twib twib;
	
	twili::twib::Request rq;
	rq.device_id = 0;
	rq.object_id = 0;
	
	if(ld->parsed()) {
		rq.command_id = 10;
		std::future<twili::twib::Response> rs_future = twib.SendRequest(rq);
		rs_future.wait();
		twili::twib::Response rs = rs_future.get();
		log(INFO, "got back response: code 0x%x", rs.result_code);
		log(INFO, "payload size: 0x%x", rs.payload.size());
		std::string err;
		msgpack11::MsgPack obj = msgpack11::MsgPack::parse(std::string(rs.payload.begin(), rs.payload.end()), err);
		std::vector<std::array<std::string, 3>> rows;
		rows.push_back({"Device ID", "Nickname", "Firmware Version"});
		for(msgpack11::MsgPack device : obj.array_items()) {
			uint32_t device_id = device["device_id"].uint32_value();
			msgpack11::MsgPack ident = device["identification"];
			auto nickname = ident["device_nickname"].string_value();
			auto version = ident["firmware_version"].binary_items();
			std::string fw_version((char*) version.data() + 0x68);

			std::stringstream device_id_stream;
			device_id_stream << std::setfill('0') << std::setw(8) << std::hex << device_id;
			rows.push_back({device_id_stream.str(), nickname, fw_version});
		}
		std::array<int, 3> lengths = {0, 0, 0};
		for(auto r : rows) {
			for(int i = 0; i < 3; i++) {
				if(r[i].size() > lengths[i]) {
					lengths[i] = r[i].size();
				}
			}
		}
		for(auto r : rows) {
			printf("%-*s | %-*s | %-*s\n", lengths[0], r[0].c_str(), lengths[1], r[1].c_str(), lengths[2], r[2].c_str());
		}

		return 0;
	}

	//device_id = std::stoull(device_id_str, NULL, 16);
	
	return 0;
}
