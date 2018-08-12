#pragma once

#include<vector>

#include<msgpack11.hpp>

#include "RemoteObject.hpp"
#include "ITwibPipeReader.hpp"

namespace twili {
namespace twib {

struct ProcessListEntry {
	uint64_t process_id;
	uint32_t result;
	uint64_t title_id;
	char process_name[12];
	uint32_t mmu_flags;
};

struct RunResult {
	uint64_t pid;
	ITwibPipeReader tp_stdout;
	ITwibPipeReader tp_stderr;
};

class ITwibDeviceInterface {
 public:
	ITwibDeviceInterface(RemoteObject obj);

	RunResult Run(std::vector<uint8_t> executable);
	void Reboot();
	std::vector<uint8_t> CoreDump(uint64_t process_id);
	void Terminate(uint64_t process_id);
	std::vector<ProcessListEntry> ListProcesses();
	msgpack11::MsgPack Identify();
	std::vector<std::string> ListNamedPipes();
	ITwibPipeReader OpenNamedPipe(std::string name);
 private:
	RemoteObject obj;
};

} // namespace twib
} // namespace twili
