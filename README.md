# Twili

[![AppVeyor build status](https://ci.appveyor.com/api/projects/status/vfgm34q4lf29ctaq/branch/master?svg=true)](https://ci.appveyor.com/project/misson20000/twili/branch/master)
[![Travis build status](https://travis-ci.org/misson20000/twili.svg?branch=master)](https://travis-ci.org/misson20000/twili)

Twili is a debug monitor/bridge for homebrew applications running on the Nintendo Switch.

The debug monitor aspect of it aims to provide a sane solution for stdio and logging, and to ease development by generating crash reports.
The debug bridge aspect aims to provide similar utilities to [ADB](https://developer.android.com/studio/command-line/adb), allowing fast and convenient sideloading and filesystem access.

## Supported Firmware Versions

Version | Support
-|-
1.0.0 | Tested partially working
2.0.0 | Expected working
2.1.0 - 2.3.0 | Expected working
3.0.0 | Tested working
3.0.1 - 4.0.0 | Expected working
4.0.1 | Tested working
4.1.0 | Expected working
5.0.0 | Tested working
5.0.1 - 5.1.0 | Expected working
6.0.0 | Unknown

# Installation

The currently recommended way to use Twili is to launch it via Atmosphère-pm.boot2. You will need a recent (after [b365965](https://github.com/Atmosphere-NX/Atmosphere/commit/b365065a2d9048eb4a3263690769bcf0ec34322d)) build of both Atmosphère-pm and Atmosphère-loader.
I suggest that you [build](#building) the latest version of Twili from source, but I understand that lots of people don't want to do that. You can download the latest version of `twili.zip` from the [latest test build](https://github.com/misson20000/twili/releases/tag/t7). Extract it to the root of your microSD card, letting the `atmosphere` directory tree merge with any existing directory tree.

You will also need `twib` and `twibd`. These are somewhat easier to build than twili, if you don't have a libtransistor toolchain set up.

If you're running Arch Linux, you can install [twib-git from the AUR](https://aur.archlinux.org/packages/twib-git/). This package installs twibd as a systemd service, so you're ready to launch Twili and start using `twib`.

If you're not running Arch Linux, you can download `twib` and `twibd` from the [latest test build](https://github.com/misson20000/twili/releases/tag/t7). Twibd will likely need to be run as root to grant permissions for USB devices and /var/run, but with udev rules, you can run it as a normal user. You will need to leave twibd running in the background.

Windows support is experimental.

## Twib Usage

```
Twili debug monitor client
Usage: twib [OPTIONS] SUBCOMMAND

Options:
  -h,--help                   Print this help message and exit
  -d,--device DeviceId (Env:TWIB_DEVICE)
                              Use a specific device
  -v,--verbose                Enable debug logging
  -f,--frontend TEXT in {tcp,unix} (Env:TWIB_FRONTEND)
  -P,--unix-path TEXT (Env:TWIB_UNIX_FRONTEND_PATH)
                              Path to the twibd UNIX socket
  -p,--tcp-port UINT (Env:TWIB_TCP_FRONTEND_PORT)
                              Port for the twibd TCP socket

Subcommands:
  list-devices                List devices
  run                         Run an executable
  reboot                      Reboot the device
  coredump                    Make a coredump of a crashed process
  terminate                   Terminate a process on the device
  ps                          List processes on the device
```

# Building

## Twili

First, you will need to install libtransistor. Download the [latest release](https://github.com/reswitched/libtransistor/releases/latest), extract it somewhere, and set `LIBTRANSISTOR_HOME` to that path. In order to build KIP1 binaries, you will need to use elf2kip form the `elf2kip-segments` branch of [my switch-tools fork](https://github.com/misson20000/switch-tools/tree/elf2kip-segments). After that, building Twili should be as simple as running `make`. If you want to use twili_launcher, `twili_launcher.kip` will be available in `build/`. Otherwise, if you're using Atmosphère-boot2, you can copy the files from `build/atmosphere/` to the `/atmosphere/` directory on your microSD card.

## Twib

Twib currently uses a CMake-based build system. I recommend building out-of-tree. You may want to enable systemd integration with `cmake -DWITH_SYSTEMD=ON`.

```
$ cd twib/
$ mkdir build
$ cd build
$ cmake -G "Unix Makefiles" ..
$ make -j4
```

# Details

## Twili Launcher

Twili Launcher is a small application designed to spawn the process that Twili runs in. Its original purpose was to be launched through nspwn, and allow the Twili process to escape getting killed when the applet closes. It now finds a purpose in helping Twili escape the memory limitations of a builtin process.

## Twib

Twib is the workstation-side tool, consisting of two parts: `twibd`, and `twib`. `twibd` is the server side, which communicates with Twili over USB or sockets, and multiplexes access between multiple instances of the `twib` client-side. The `twibd` server-side has a number of backends, which handle talking to Twili. It also has a number of frontends, which accept connections from the `twib` client. On UNIX systems, this is a UNIX socket. On Windows systems, this is a named pipe.

![Twib block diagram](doc/twib_diagram.svg)

The frontend and backend each run in their own threads to simplify synchronization.

## Protocol Overview

Twili will provide a USB interface with class 0xFF, subclass 0x01, and protocol 0x00.
This interface shall have four bulk endpoints, two IN, and two OUT.
Communication happens as requests and responses over these endpoints.

A request contains an id for the remote bridge object that should handle the request,
a command id to determine which function the remote object is to perform,
a tag to associate the request with its response,
and an arbitrarily sized payload containing any additional data needed for the operation.

The request header is sent over the first OUT endpoint as a single transfer.
Next, if the request header indicates that a payload is present, the data is sent
over the second OUT endpoint.
Finally, if the request header indicates that there are objects being sent, the
IDs of each object are sent over the same (second) OUT endpoint in a single transfer.

Once the request is processed, a response header is sent over the first IN endpoint.
If the response header indicates that a payload is present, it is sent over the second
IN endpoint. If the response header indicates that there are objects being sent,
the IDs of each object are sent over the same (second) IN endpoint in a single transfer.

Upon receiving a request, the specified function is invoked on the specified object.
When the operation completes, as either a success or a failure, a response is sent.
The response's `tag` field must match the tag of the request that started the operation.
Any operation may be executed asynchronously, meaning that responses may be sent in a
different order from the requests that caused them.

If a response includes any references to bridge objects, their IDs are encoded in the
payload as indexes into the array of object IDs sent with the response.

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
	u32 object_count;
	
	// payload
	u8 payload[payload_size];

	// object IDs
	u32 object_ids[object_count];
};
```

Initially, only the object with id 0 exists on the device. It represents `ITwibDeviceInterface`.
Every object responds to command `0xffffffff`, which destroys the object, except for
`ITwibDeviceInterface` which handles this command by destroying every object except itself.
This is invoked when a device is first detected by twibd, to indicate to the device that
any objects that may have previously existed have had their references lost and should be
cleaned up.

### ITwibDeviceInterface

#### Command ID 10: `RUN`

The request payload for this command is a complete executable file, in `NRO` format. Upon successful completion, the response payload is a `u64 process_id`, followed by three object IDs corresponding to one [ITwibPipeWriter](#ITwibPipeWriter) and two [ITwibPipeReaders](#ITwibPipeReader).

##### Request
```
u8 executable[];
```

##### Response
```
u64 process_id;
u32 stdin_object_index; // ITwibPipeWriter
u32 stdout_object_index; // ITwibPipeReader
u32 stderr_object_index; // ITwibPipeReader
```

#### Command ID 11: `REBOOT`

This request has no payload, and does not send any response on success, because it brings the USB interface down with it.

#### Command ID 12: `COREDUMP`

Takes a PID, sends an ELF core dump file as a response.

##### Request
```
u64 pid;
```

##### Response
```
u8 elf_file[];
```

#### Command ID 13: `TERMINATE`

The request payload for this command is a single `u64 process_id`. It does not have a response payload.

##### Request
```
u64 process_id;
```

#### Command ID 14: `LIST_PROCESSES`

This command has no request payload. The response payload is an array of `ProcessReport`s.

##### Response
```
struct ProcessReport {
	u64 process_id;
	u32 result_code;
	u64 title_id;
	u8 process_name[12];
	u32 mmu_flags;
} reports[];
```

#### Command ID 15: `UPGRADE_TWILI`

Stubbed.

#### Command ID 16: `IDENTIFY`

This command has no request payload. The response payload is a MessagePack object containing details about the device.

##### Response

MessagePack object containing these keys:

- `service` = `twili`
- `protocol_version` = `1`
- `firmware_version` (binary data fetched from `set:sys`#3)
- `serial_number` (binary data fetched from `set:cal`#9)
- `bluetooth_bd_address` (binary data fetched from `set:cal`#0)
- `wireless_lan_mac_address` (binary data fetched from `set:cal`#6)
- `device_nickname` (string fetched from `set:sys`#77)
- `mii_author_id` (binary data fetched from `set:sys`#90)

#### Command ID 17: `LIST_NAMED_PIPES`

Takes empty request payload, reponds with a list of pipe names.

##### Response
```
u32 pipe_count;
struct pipe_name {
  u32 length;
	char name[length];
} pipe_names[pipe_count];
```

#### Command ID 18: `OPEN_NAMED_PIPE`

Takes the name of a pipe, returns an [ITwibPipeReader](#ITwibPipeReader).

##### Request
```
char pipe_name[];
```

##### Response
```
u32 reader_object_index; // ITwibPipeReader
```
