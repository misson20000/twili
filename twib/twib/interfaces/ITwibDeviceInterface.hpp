#pragma once

#include<vector>

#include<msgpack11.hpp>

#include "../RemoteObject.hpp"
#include "ITwibPipeWriter.hpp"
#include "ITwibPipeReader.hpp"
#include "ITwibProcessMonitor.hpp"

namespace twili {
namespace twib {

struct ProcessListEntry {
	uint64_t process_id;
	uint32_t result;
	uint64_t title_id;
	char process_name[12];
	uint32_t mmu_flags;
};

class ITwibDeviceInterface {
 public:
	ITwibDeviceInterface(std::shared_ptr<RemoteObject> obj);

	ITwibProcessMonitor CreateMonitoredProcess(std::string type);
	void Reboot();
	std::vector<uint8_t> CoreDump(uint64_t process_id);
	void Terminate(uint64_t process_id);
	std::vector<ProcessListEntry> ListProcesses();
	msgpack11::MsgPack Identify();
	std::vector<std::string> ListNamedPipes();
	ITwibPipeReader OpenNamedPipe(std::string name);
	msgpack11::MsgPack GetMemoryInfo();
 private:
	std::shared_ptr<RemoteObject> obj;
};

} // namespace twib
} // namespace twili
