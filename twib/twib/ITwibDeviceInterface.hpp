#pragma once

#include<vector>

#include "RemoteObject.hpp"

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
	ITwibDeviceInterface(RemoteObject obj);

	uint64_t Run(std::vector<uint8_t> executable);
	std::vector<ProcessListEntry> ListProcesses();
 private:
	RemoteObject obj;
};

} // namespace twib
} // namespace twili
