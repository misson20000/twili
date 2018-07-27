#include "ITwibDeviceInterface.hpp"

#include "Protocol.hpp"
#include "Logger.hpp"

namespace twili {
namespace twib {

ITwibDeviceInterface::ITwibDeviceInterface(RemoteObject obj) : obj(obj) {
}

RunResult ITwibDeviceInterface::Run(std::vector<uint8_t> executable) {
	Response rs = obj.SendSyncRequest(protocol::ITwibDeviceInterface::Command::RUN, executable);
	if(rs.payload.size() < sizeof(uint64_t)) {
		LogMessage(Fatal, "response size invalid");
		exit(1);
	}
	struct {
		uint64_t pid;
		uint32_t tp_stdin;
		uint32_t tp_stdout;
		uint32_t tp_stderr;
	} rss = *(decltype(rss)*) rs.payload.data();
	return {rss.pid, obj.CreateSiblingFromId(rss.tp_stdout), obj.CreateSiblingFromId(rss.tp_stderr)};
}

void ITwibDeviceInterface::Reboot() {
	obj.SendSyncRequest(protocol::ITwibDeviceInterface::Command::REBOOT);
}

std::vector<uint8_t> ITwibDeviceInterface::CoreDump(uint64_t process_id) {
    uint8_t *process_id_bytes = (uint8_t*) &process_id;
    return obj.SendSyncRequest(protocol::ITwibDeviceInterface::Command::COREDUMP, std::vector<uint8_t>(process_id_bytes, process_id_bytes + sizeof(process_id))).payload;
}

void ITwibDeviceInterface::Terminate(uint64_t process_id) {
	uint8_t *process_id_bytes = (uint8_t*) &process_id;
	obj.SendSyncRequest(protocol::ITwibDeviceInterface::Command::TERMINATE, std::vector<uint8_t>(process_id_bytes, process_id_bytes + sizeof(process_id)));
}

std::vector<ProcessListEntry> ITwibDeviceInterface::ListProcesses() {
	Response rs = obj.SendSyncRequest(protocol::ITwibDeviceInterface::Command::LIST_PROCESSES);
	ProcessListEntry *first = (ProcessListEntry*) rs.payload.data();
	return std::vector<ProcessListEntry>(first, first + (rs.payload.size() / sizeof(ProcessListEntry)));
}

msgpack11::MsgPack ITwibDeviceInterface::Identify() {
	Response rs = obj.SendSyncRequest(protocol::ITwibDeviceInterface::Command::IDENTIFY);
	std::string err;
	return msgpack11::MsgPack::parse(std::string(rs.payload.begin(), rs.payload.end()), err);
}

} // namespace twib
} // namespace twil
