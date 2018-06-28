#include "ITwibDeviceInterface.hpp"

#include "Protocol.hpp"
#include "Logger.hpp"

namespace twili {
namespace twib {

ITwibDeviceInterface::ITwibDeviceInterface(RemoteObject obj) : obj(obj) {
}

uint64_t ITwibDeviceInterface::Run(std::vector<uint8_t> executable) {
	std::future<twili::twib::Response> rs_future = obj.SendRequest((uint32_t) protocol::ITwibDeviceInterface::Command::RUN, executable);
	rs_future.wait();
	twili::twib::Response rs = rs_future.get();
	if(rs.payload.size() < sizeof(uint64_t)) {
		log(FATAL, "response size invalid");
		exit(1);
	}
	return *(uint64_t*) rs.payload.data();
}

std::vector<ProcessListEntry> ITwibDeviceInterface::ListProcesses() {
	std::future<twili::twib::Response> rs_future = obj.SendRequest((uint32_t) protocol::ITwibDeviceInterface::Command::LIST_PROCESSES, std::vector<uint8_t>());
	rs_future.wait();
	twili::twib::Response rs = rs_future.get();

	ProcessListEntry *first = (ProcessListEntry*) rs.payload.data();
	return std::vector<ProcessListEntry>(first, first + (rs.payload.size() / sizeof(ProcessListEntry)));
}

} // namespace twib
} // namespace twil
