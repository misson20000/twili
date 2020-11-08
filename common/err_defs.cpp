//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2020 misson20000 <xenotoad@xenotoad.net>
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

#include<array>
#include<algorithm>

#include "err_defs.hpp"
#include "err.hpp"

namespace twili {

#define describe(vis, r, description, help) ResultDescription {r, ResultVisibility::vis, #r, description, help}

static auto descriptions = std::array {
	describe(User,     TWILI_ERR_INVALID_NRO, "Invalid NRO file", nullptr),
	describe(User,     TWILI_ERR_NO_CRASHED_PROCESSES, "There are no crashed processes", nullptr),
	describe(Internal, TWILI_ERR_FATAL_USB_TRANSFER, "Unrecoverable USB error", "The device encountered a fatal error while processing USB transfers."),
	describe(Internal, TWILI_ERR_UNRECOGNIZED_PID, "Unrecognized PID", "A non-monitored process tried to open an HBABI shim."),
	describe(Internal, TWILI_ERR_UNRECOGNIZED_HANDLE_PLACEHOLDER, "Unrecognized handle placeholder", "A monitored process requested an invalid handle while translating HBABI entries."),
	describe(User,     TWILI_ERR_IO_ERROR, "IO Error", "A socket was closed or a file operation failed unexpectedly."),
	describe(User,     TWILI_ERR_WONT_DEBUG_SELF, "Won't debug self", "The sysmodule tried to debug itself."),
	describe(User,     TWILI_ERR_INVALID_PIPE, "Invalid pipe", "A monitored process tried to open a pipe with a bad index."),
	describe(User,     TWILI_ERR_EOF, "End-of-file", "The other end of a pipe was closed."),
	describe(Internal, TWILI_ERR_INVALID_PIPE_STATE, "Invalid pipe state", "An operation was attempted on a busy end of a pipe."),
	describe(User,     TWILI_ERR_PIPE_ALREADY_EXISTS, "Pipe already exists", "A named pipe already exists with the requested name."),
	describe(User,     TWILI_ERR_NO_SUCH_PIPE, "No such pipe", "There is no named pipe with the requested name."),
	describe(User,     TWILI_ERR_ALREADY_WAITING, "Already waiting", "A bridge object was asked to wait on an event that there is already an outstanding wait request for."),
	describe(Internal, TWILI_ERR_FATAL_BRIDGE_STATE, "Fatal bridge state", nullptr),
	describe(Internal, TWILI_ERR_APPLET_SHIM_UNKNOWN_MODE, "Unknown applet shim mode", nullptr),
	describe(Internal, TWILI_ERR_APPLET_SHIM_WRONG_MODE, "Wrong applet shim mode", nullptr),
	describe(Internal, TWILI_ERR_APPLET_SHIM_NO_COMMANDS_LEFT, "No applet shim commands left", nullptr),
	describe(Internal, TWILI_ERR_APPLET_TRACKER_INVALID_STATE, "Applet tracker encountered invalid state", nullptr),
	describe(Internal, TWILI_ERR_TRACKER_NO_PROCESSES_CREATED, "No processes created by tracker", nullptr),
	describe(Internal, TWILI_ERR_MONITORED_PROCESS_ALREADY_ATTACHED, "Monitored process already attached", nullptr),
	describe(Api,      TWILI_ERR_UNRECOGNIZED_MONITORED_PROCESS_TYPE, "Unrecognized monitored process type", "CreateMonitoredProcess was called with an invalid type."),
	describe(Internal, TWILI_ERR_TOO_MANY_MODULES, "Too many modules", "An ECS process was requested to be launched with too many modules."),
	describe(Api,      TWILI_ERR_INTERRUPTED, "Interruped", "Another request was received to wait on this event"),
	describe(Internal, TWILI_ERR_NO_MODULES, "No modules", "An ECS process was requested to be launched with no modules."),
	describe(User,     TWILI_ERR_UNKNOWN_FILESYSTEM, "Unknown filesystem", "Attempted to open an unrecognized filesystem."),
	describe(Api,      TWILI_ERR_INVALID_PROCESS_STATE, "Invalid process state", "The process is in an invalid state for the requested operation."),
	describe(Internal, TWILI_ERR_UNSPECIFIED, "Unspecified", nullptr),
	describe(User,     TWILI_ERR_SYSMODULE_BEING_DEBUGGED, "Sysmodule being debugged", "An operation was requested that depends on a sysmodule that is currently being debugged."),
	describe(Internal, TWILI_ERR_INVALID_DEBUGGER_STATE, "Invalid debugger state", "The kernel has behaved unexpectedly while performing a debugger operation."),
	describe(Internal, TWILI_ERR_SYSTEM_VERSION_NOT_SET, "System version not set", "The system version has not been determined yet."),
	describe(Internal, TWILI_ERR_SYSTEM_VERSION_ALREADY_SET, "System version already set", "Attempted to set the system version twice."),
	describe(User,     TWILI_ERR_OPERATION_NOT_SUPPORTED_FOR_SYSTEM_VERSION, "Operation not supported for system version", "An operation was requested that is not supported on the current system version."),
	describe(Internal, TWILI_ERR_TODO, "Todo", "The requsted operation has not been implemented yet."),
	describe(Internal, TWILI_ERR_USB_SERIAL_INIT_FAILED, "USB serial init failed", "An error was encountered while initializing USB serial"),
	describe(Api,      TWILI_ERR_NO_LONGER_REQUESTED_TO_LAUNCH, "Process no longer requested to launch", "A process launch was cancelled asynchronously."),
	describe(Internal, TWILI_ERR_ECS_CONFUSED, "ECS confused", "ECS encountered an invalid state."),

	describe(Api,      TWILI_ERR_PROTOCOL_UNRECOGNIZED_OBJECT, "Unrecognized object", "The bridge did not recognize the requested object."),
	describe(Api,      TWILI_ERR_PROTOCOL_UNRECOGNIZED_FUNCTION, "Unrecognized function", "The object did not recognize the requested function."),
	describe(User,     TWILI_ERR_PROTOCOL_TRANSFER_ERROR, "Transfer error", "An error was encountered while transferring data to or from the bridge."),
	describe(User,     TWILI_ERR_PROTOCOL_UNRECOGNIZED_DEVICE, "Unrecognized device", "The bridge daemon did not recognize the requested device."),
	describe(Api,      TWILI_ERR_PROTOCOL_BAD_REQUEST, "Bad request", "The request was malformed."),
	describe(Api,      TWILI_ERR_PROTOCOL_BAD_RESPONSE, "Bad response", "The response was malformed."),
};

ResultDescription ResultDescription::Lookup(uint32_t code) {
	auto i = std::lower_bound(descriptions.begin(), descriptions.end(), code, [](const ResultDescription &d, uint32_t value) {
		return d.code < value;
	});

	if(i == descriptions.end() || i->code != code) {
		return ResultDescription {
			.code = code,
			.visibility = ResultVisibility::Internal,
			.name = "Unknown",
			.description = "Unknown",
			.help = nullptr
		};
	} else {
		return *i;
	}
}

} // namespace twili
