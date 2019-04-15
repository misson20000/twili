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
	uint32_t object_count;
};

const int VERSION = 2;

class ITwibMetaInterface {
 public:
	enum class Command : uint32_t {
		LIST_DEVICES = 10,
		CONNECT_TCP = 11,
	};
};

class ITwibDeviceInterface {
 public:
	enum class Command : uint32_t {
		CREATE_MONITORED_PROCESS = 10,
		REBOOT = 11,
		COREDUMP = 12,
		TERMINATE = 13,
		LIST_PROCESSES = 14,
		UPGRADE_TWILI = 15,
		IDENTIFY = 16,
		LIST_NAMED_PIPES = 17,
		OPEN_NAMED_PIPE = 18,
		OPEN_ACTIVE_DEBUGGER = 19,
		GET_MEMORY_INFO = 20,
		PRINT_DEBUG_INFO = 21,
		LAUNCH_UNMONITORED_PROCESS = 22,
		OPEN_FILESYSTEM_ACCESSOR = 23,
		WAIT_TO_DEBUG_APPLICATION = 24,
		WAIT_TO_DEBUG_TITLE = 25,
	};
};

class ITwibPipeReader {
 public:
	enum class Command : uint32_t {
		READ = 10,
	};
};

class ITwibPipeWriter {
 public:
	enum class Command : uint32_t {
		WRITE = 10,
		CLOSE = 11,
	};
};

class ITwibDebugger {
 public:
	enum class Command : uint32_t {
		QUERY_MEMORY = 10,
		READ_MEMORY = 11,
		WRITE_MEMORY = 12,
		LIST_THREADS = 13,
		GET_DEBUG_EVENT = 14,
		GET_THREAD_CONTEXT = 15,
		BREAK_PROCESS = 16,
		CONTINUE_DEBUG_EVENT = 17,
		SET_THREAD_CONTEXT = 18,
		GET_NSO_INFOS = 19,
		WAIT_EVENT = 20,
		GET_TARGET_ENTRY = 21,
		LAUNCH_DEBUG_PROCESS = 22,
		GET_NRO_INFOS = 24,
	};
};

class ITwibProcessMonitor {
 public:
	enum class Command : uint32_t {
		LAUNCH = 10,
		LAUNCH_SUSPENDED = 11,
		TERMINATE = 12,
		APPEND_CODE = 13,
		OPEN_STDIN = 14,
		OPEN_STDOUT = 15,
		OPEN_STDERR = 16,
		WAIT_STATE_CHANGE = 17,
	};
};

class ITwibFilesystemAccessor {
 public:
	enum class Command : uint32_t {
		CREATE_FILE = 10,
		DELETE_FILE = 11,
		CREATE_DIRECTORY = 12,
		DELETE_DIRECTORY = 13,
		GET_ENTRY_TYPE = 17,
		OPEN_FILE = 18,
		OPEN_DIRECTORY = 19,
	};
};

class ITwibFileAccessor {
 public:
	enum class Command : uint32_t {
		READ = 10,
		WRITE = 11,
		FLUSH = 12,
		SET_SIZE = 13,
		GET_SIZE = 14,
	};
};

class ITwibDirectoryAccessor {
 public:
	enum class Command : uint32_t {
		READ = 10,
		GET_ENTRY_COUNT = 11,
	};
};

} // namespace protocol
} // namespace twili
