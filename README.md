# Twili

Twili is a debug monitor/bridge for homebrew applications running on the Nintendo Switch.

The debug monitor aspect of it aims to provide a sane solution for stdio and logging, and to ease development by generating crash reports.
The debug bridge aspect aims to provide similar utilities to [ADB](https://developer.android.com/studio/command-line/adb), allowing fast and convenient sideloading and filesystem access.

# Twib

Twib is the workstation-side tool, consisting of two parts: `twibd`, and `twib`. `twibd` is the server side, which communicates with Twili over USB or sockets, and multiplexes access between multiple instances of the `twib` client-side. The `twibd` server-side has a number of backends, which handle talking to Twili. It also has a number of frontends, which accept connections from the `twib` client. On UNIX systems, this is a UNIX socket. On Windows systems, this is a named pipe.

![Twib block diagram](doc/twib_diagram.svg)

The frontend and backend each run in their own threads to simplify synchronization.

## Protocol

Twili will provide a USB interface with class 0xFF, subclass 0x01, and protocol 0x00.
This interface shall have four bulk endpoints, two IN, and two OUT.
Communication happens as requests and responses over these endpoints.

A request contains an address for the remote object that should handle the request,
a command id to determine which function the remote object is to perform,
a tag to associate the request with its response,
and an arbitrarily sized payload containing any additional data needed for the operation.

The request header is sent over the first OUT endpoint as a single transfer,
and then, if the request header indicates that a payload is present, the data is sent
over the second OUT endpoint.

Once the request is processed, a response header is sent over the first IN endpoint, and
data is sent over the second IN endpoint.

Upon receiving a request, the specified function is invoked on the specified object.
When the operation completes, as either a success or a failure, a response is sent.
The response's `tag` field must match the tag of the request that started the operation.
Any operation may be executed asynchronously, meaning that responses may be sent in a
different order from the requests that caused them.

Requests and responses share a similar format.

```
struct message {
	// header
	union {
		u32 device_id; // for transport between twib and twibd
		u32 client_id; // for transport between twibd and twili
	};
	u32 object_id;
	union {
		u32 command_id; // request
		u32 result_code; // response
	};
	u32 tag;
	u64 payload_size;
	
	// payload
	u8 payload[payload_size];
};
```

Initially, only the pipe with id 0 exists on the device. It represents `ITwibInterface`, which responds to the following commands:

### Command ID 10: `RUN`

The request payload for this command is a complete executable file, in either `NRO` or `ELF` format. Upon successful completion, the response payload is a `u64 process_id` followed by three `u32 object_id`s corresponding to an `ITwibPipeWriter` representing `stdin`, and two `ITwibPipeReader`s representing `stdout` and `stderr`.

#### Request
```
u8 executable[];
```

#### Response
```
u64 process_id;
u32 stdin_object_id;
u32 stdout_object_id;
u32 stderr_object_id;
```

### Command ID 11: `REBOOT`

This request has no payload, and does not send any response on success, because it brings the USB interface down with it.

### Command ID 12: `COREDUMP`

TBD

### Command ID 13: `TERMINATE`

The request payload for this command is a single `u64 process_id`. It does not have a response payload.

#### Request
```
u64 process_id;
```

### Command ID 14: `LIST_PROCESSES`

This command has no request payload. The response payload is an array of `ProcessReport`s.

#### Response
```
struct ProcessReport {
	u64 process_id;
	u32 result_code;
	u64 title_id;
	u8 process_name[12];
	u32 mmu_flags;
} reports[];
```

### Command ID 15: `UPGRADE_TWILI`

TBD

### Command ID 16: `IDENTIFY`

This command has no request payload. The response payload is a MessagePack object containing details about the device.

#### Response

MessagePack object containing these keys:

- `service` = `twili`
- `protocol_version` = `1`
- `firmware_version` (binary data fetched from `set:sys`#3)
- `serial_number` (binary data fetched from `set:cal`#9)
- `bluetooth_bd_address` (binary data fetched from `set:cal`#0)
- `wireless_lan_mac_address` (binary data fetched from `set:cal`#6)
- `device_nickname` (string fetched from `set:sys`#77)
- `mii_author_id` (binary data fetched from `set:sys`#90)
