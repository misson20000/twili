#include "TCPBackend.hpp"

#include "platform.hpp"

#include "Twibd.hpp"

#include<netdb.h>

namespace twili {
namespace twibd {
namespace backend {

TCPBackend::TCPBackend(Twibd *twibd) :
	twibd(twibd) {

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	struct addrinfo *res = 0;
	int err = getaddrinfo("10.0.0.195", "15152", &hints, &res);
	if(err != 0) {
		LogMessage(Error, "failed to resolve remote socket address");
		exit(1);
	}
	int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(fd == -1) {
		LogMessage(Error, "%s", strerror(errno));
		exit(1);
	}
	if(connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
		LogMessage(Error, "%s", strerror(errno));
		exit(1);
	}
	freeaddrinfo(res);

	connections.emplace_back(std::make_shared<twibc::MessageConnection<Device>>(fd, this))->obj->Begin();

	event_thread = std::thread(&TCPBackend::event_thread_func, this);
}

TCPBackend::~TCPBackend() {
	event_thread_destroy = true;
	event_thread.join();
}

TCPBackend::Device::Device(twibc::MessageConnection<Device> &mc, TCPBackend *backend) :
	backend(backend),
	connection(mc) {
}

TCPBackend::Device::~Device() {
}

void TCPBackend::Device::Begin() {
	SendRequest(Request(std::shared_ptr<Client>(), 0x0, 0x0, (uint32_t) protocol::ITwibDeviceInterface::Command::IDENTIFY, 0xFFFFFFFF, std::vector<uint8_t>()));
}

void TCPBackend::Device::IncomingMessage(protocol::MessageHeader &mh, util::Buffer &payload, util::Buffer &object_ids) {
	response_in.device_id = device_id;
	response_in.client_id = mh.client_id;
	response_in.object_id = mh.object_id;
	response_in.result_code = mh.result_code;
	response_in.tag = mh.tag;
	response_in.payload = std::vector<uint8_t>(payload.Read(), payload.Read() + payload.ReadAvailable());
	
	// create BridgeObjects
	response_in.objects.resize(mh.object_count);
	for(uint32_t i = 0; i < mh.object_count; i++) {
		uint32_t id;
		if(!object_ids.Read(id)) {
			LogMessage(Error, "not enough object IDs");
			return;
		}
		response_in.objects[i] = std::make_shared<BridgeObject>(*backend->twibd, mh.device_id, id);
	}

	// remove from pending requests
	pending_requests.remove_if([this](WeakRequest &r) {
			return r.tag == response_in.tag;
		});
	
	if(response_in.client_id == 0xFFFFFFFF) { // identification meta-client
		Identified(response_in);
	} else {
		backend->twibd->PostResponse(std::move(response_in));
	}
}

void TCPBackend::Device::Identified(Response &r) {
	LogMessage(Debug, "got identification response back");
	LogMessage(Debug, "payload size: 0x%x", r.payload.size());
	if(r.result_code != 0) {
		LogMessage(Warning, "device identification error: 0x%x", r.result_code);
		deletion_flag = true;
		return;
	}
	std::string err;
	msgpack11::MsgPack obj = msgpack11::MsgPack::parse(std::string(r.payload.begin(), r.payload.end()), err);
	identification = obj;
	device_nickname = obj["device_nickname"].string_value();
	std::vector<uint8_t> sn = obj["serial_number"].binary_items();
	serial_number = std::string(sn.begin(), sn.end());

	LogMessage(Info, "nickname: %s", device_nickname.c_str());
	LogMessage(Info, "serial number: %s", serial_number.c_str());
	
	device_id = std::hash<std::string>()(serial_number);
	LogMessage(Info, "assigned device id: %08x", device_id);
	ready_flag = true;
}

void TCPBackend::Device::SendRequest(const Request &&r) {
	protocol::MessageHeader mhdr;
	mhdr.client_id = r.client ? r.client->client_id : 0xffffffff;
	mhdr.object_id = r.object_id;
	mhdr.command_id = r.command_id;
	mhdr.tag = r.tag;
	mhdr.payload_size = r.payload.size();
	mhdr.object_count = 0;

	pending_requests.push_back(r.Weak());

	connection.out_buffer.Write(mhdr);
	connection.out_buffer.Write(r.payload);
	/* TODO: request objects
	std::vector<uint32_t> object_ids(r.objects.size(), 0);
	std::transform(
		r.objects.begin(), r.objects.end(), object_ids.begin(),
		[](auto const &object) {
			return object->object_id;
		});
	connection.out_buffer.Write(object_ids); */
	connection.PumpOutput();
}

void TCPBackend::event_thread_func() {
	fd_set readfds;
	fd_set writefds;
	SOCKET max_fd = 0;
	while(!event_thread_destroy) {
		LogMessage(Debug, "tcp backend event thread loop");
		
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		
		// add device connections
		for(auto &c : connections) {
			if(c->obj->ready_flag && !c->obj->added_flag) {
				twibd->AddDevice(c->obj);
				c->obj->added_flag = true;
			}
			
			max_fd = std::max(max_fd, c->fd);
			FD_SET(c->fd, &readfds);
			
			if(c->out_buffer.ReadAvailable() > 0) { // only add to writefds if we need to write
				FD_SET(c->fd, &writefds);
			}
		}

		if(select(max_fd + 1, &readfds, &writefds, NULL, NULL) < 0) {
			LogMessage(Fatal, "failed to select file descriptors: %s", NetErrStr());
			exit(1);
		}
		
		// pump i/o
		for(auto mci = connections.begin(); mci != connections.end(); mci++) {
			std::shared_ptr<twibc::MessageConnection<Device>> &mc = *mci;
			if(FD_ISSET(mc->fd, &writefds)) {
				mc->PumpOutput();
			}
			if(FD_ISSET(mc->fd, &readfds)) {
				LogMessage(Debug, "incoming data for device %x", mc->obj->device_id);
				mc->PumpInput();
			}
		}

		for(auto i = connections.begin(); i != connections.end(); ) {
			(*i)->Process();
			
			if((*i)->obj->deletion_flag) {
				if((*i)->obj->added_flag) {
					twibd->RemoveDevice((*i)->obj);
				}
				i = connections.erase(i);
				continue;
			}

			i++;
		}
	}
}

} // namespace backend
} // namespace twibd
} // namespace twili
