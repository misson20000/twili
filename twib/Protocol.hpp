#pragma once

#include<stdint.h>

namespace twili {
namespace protocol {
	
struct MessageHeader {
	union {
		uint32_t device_id;
		uint32_t client_id;
	};
	uint32_t object_id;
	union {
		uint32_t command_id;
		uint32_t result_code;
	};
	uint32_t tag;
	uint64_t payload_size;
};

const int VERSION = 2;

class ITwibMetaInterface {
 public:
	enum class Command : uint32_t {
		LIST_DEVICES = 10,
	};
};

class ITwibDeviceInterface {
 public:
	enum class Command : uint32_t {
		RUN = 10,
		REBOOT = 11,
		COREDUMP = 12,
		TERMINATE = 13,
		LIST_PROCESSES = 14,
		UPGRADE_TWILI = 15,
		IDENTIFY = 16,
	};
};

class ITwibPipeReader {
 public:
	enum class Command : uint32_t {
		READ = 10,
	};
};

} // namespace protocol
} // namespace twili
