# Twili

[![AppVeyor build status](https://ci.appveyor.com/api/projects/status/vfgm34q4lf29ctaq/branch/master?svg=true)](https://ci.appveyor.com/project/misson20000/twili/branch/master)
[![Travis build status](https://travis-ci.org/misson20000/twili.svg?branch=master)](https://travis-ci.org/misson20000/twili)
[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

Twili is a debug monitor/bridge for homebrew applications running on the Nintendo Switch.

The debug monitor aspect of it aims to provide a sane solution for stdio and logging, and to ease development by generating crash reports and allowing debugging of running applications.
The debug bridge aspect aims to provide similar utilities to [ADB](https://developer.android.com/studio/command-line/adb), allowing fast and convenient sideloading.

## Table of Contents

- [Twili](#twili)
  * [Supported Firmware Versions](#supported-firmware-versions)
  * [Limitations](#limitations)
  * [Application Development](#application-development)
- [Installation](#installation)
  * [Linux](#linux)
    + [Arch Linux](#arch-linux)
    + [Other Linux Distrubutions](#other-linux-distrubutions)
  * [OSX](#osx)
  * [Windows](#windows)
- [Building From Source](#building-from-source)
  * [Twili](#twili-1)
  * [Twib](#twib)
    + [Linux / OSX](#linux---osx)
- [Twib Usage](#twib-usage)
  * [twib list-devices](#twib-list-devices)
  * [twib connect-tcp](#twib-connect-tcp)
  * [twib run](#twib-run)
  * [twib reboot](#twib-reboot)
  * [twib coredump](#twib-coredump)
  * [twib terminate](#twib-terminate)
  * [twib ps](#twib-ps)
  * [twib identify](#twib-identify)
  * [twib list-named-pipes](#twib-list-named-pipes)
  * [twib open-named-pipe](#twib-open-named-pipe)
  * [twib get-memory-info](#twib-get-memory-info)
- [Developer Details](#developer-details)
  * [Project Organization](#project-organization)
  * [Process Management](#process-management)
  * [Homebrew ABI Shim](#homebrew-abi-shim)
  * [Applet Shim](#applet-shim)
  * [Twib](#twib-1)
  * [Protocol Overview](#protocol-overview)
- [Other Tools](#other-tools)

## Supported Firmware Versions

Version | Support
-|-
1.0.0 | Tested working (limited functionality)
2.0.0 | Expected working
2.1.0 - 2.3.0 | Expected working
3.0.0 | Tested working
3.0.1 - 4.0.0 | Expected working
4.0.1 | Tested working
4.1.0 | Expected working
5.0.0 | Tested working
5.0.1 - 5.1.0 | Expected working
6.0.0 | Expected working
6.0.1 | Tested working
6.1.0 | Expected Working
6.2.0 | Tested working

## Limitations

These are temporary and are planned to be addressed.

- Applets cannot be launched without an active control applet (hbmenu open).
- Applets are library applets, not applications (this means they're limited to using memory in the APPLET pool).

## Application Development

If you're developing with libtransistor, Twili integration is automatic. Standard I/O will automatically be attached to the twib console.

If you're developing an application with libnx, try [libtwili](https://github.com/misson20000/twili-libnx) for twib stdio.
Most libnx applications are applets. If you're developing an applet (interacts with the user), make sure you use `-a` with `twib run` to run it as an applet.
If you're developing a sysmodule, you can use `twib run` without `-a` to run it as a sysmodule.

# Installation

Download the [latest release](https://github.com/misson20000/twili/releases/latest) of `twili.zip` and extract it to the root of your microSD card, letting the `atmosphere` directory merge with any existing directory tree. The only CFW that I officially support is Atmosphère. As of writing, Twili depends on a feature not yet in the latest version of Atmosphère. Running Twili on piracy firmware will make me cry, so don't do it. :cry:

You will also need to install the workstation-side tools, `twib` and `twibd`.

## Linux

### Arch Linux

If you're running Arch Linux, you can install [twib-git from the AUR](https://aur.archlinux.org/packages/twib-git/). This package installs twibd as a systemd service, so you're ready to launch Twili and start using `twib`.

### Other Linux Distrubutions

It is recommended to build twib/twibd from source. If you need a binary release though, you can download `twib_linux64` and `twibd_linux64` from the [latest release](https://github.com/misson20000/twili/releases/latest). Twibd will likely need to be run as root to grant permissions for USB devices and /var/run, but with udev rules, you can run it as a normal user. You will need to leave twibd running in the background.

## OSX

OSX users can download `twib_osx64` and `twibd_osx64` from the [latest release](https://github.com/misson20000/twili/releases/latest). Alternatively, you may build from source.

## Windows

Windows support is experimental and is being improved. Windows users may download `twib_win64.exe` and `twibd_win64.exe` from the [latest release](https://github.com/misson20000/twili/releases/latest). Boot your switch with Twili installed, then install the libusbK-based [driver package](https://github.com/misson20000/twili/tree/master/contrib/windows/driver). Run `twibd_win64.exe`, and leave it running in the background. `twib_win64.exe` should be run from the command prompt.

# Building From Source

## Twili

First, you will need to install libtransistor. Download the [latest release](https://github.com/reswitched/libtransistor/releases/latest), extract it somewhere, and set `LIBTRANSISTOR_HOME` to that path. After that, building Twili should be as simple as running `make`. To use Atmosphère-boot2 to launch Twili, you can copy the files from `build/atmosphere/` to the `/atmosphere/` directory on your microSD card.

## Twib

### Linux / OSX

You will need libusb development headers installed. On Debian-based distros, these are usually in the `libusb-1.0.0-dev` package.

Clone this repo.

```
$ git clone git@github.com:misson20000/twili.git
```

Change into the `twib/` subdirectory, and make a `build/` directory.

```
$ cd twib
$ mkdir build
$ cd build
```

Choose your configuration parameters for CMake, run CMake, and build twib/twibd.

```
$ cmake -G "Unix Makefiles" .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DWITH_SYSTEMD=ON
$ make -j4
```

Finally, you can install twib/twibd. If are using systemd and built with systemd support, you will want to enable the twibd socket.

```
$ sudo make install
$ sudo systemctl enable --now twibd.socket
```

# Twib Usage

`twib` is the command line tool for interacting with Twili. The `twibd` daemon needs to be running in order to use `twib`. On Linux systems, it is recommended to use the systemd units provided. The `twibd` daemon acts as a driver for Twili, so that you can run multiple copies of `twib` at the same time that all interact with the same device.

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
  connect-tcp                 Connect to a device over TCP
  run                         Run an executable
  reboot                      Reboot the device
  coredump                    Make a coredump of a crashed process
  terminate                   Terminate a process on the device
  ps                          List processes on the device
  identify                    Identify the device
  list-named-pipes            List named pipes on the device
  open-named-pipe             Open a named pipe on the device
  get-memory-info             Gets memory usage information from the device
```

All `twib` commands require a device to be specified, except for `list-devices` and `connect-tcp`. If no device is explicitly specified and there is exactly one device currently connected to Twib, that device will be used. Otherwise, a device must be specified by device ID (obtained from `list-devices`) via the `-d` option or the `TWIB_DEVICE` environment variable.

Detailed help on all subcommands can be obtained by running `twib <subcommand> --help`.

## twib list-devices

Lists all devices currently known to Twib.

```
$ twib list-devices
Device ID | Nickname          | Firmware Version | Bridge Type
1e462a6e  | m20k's Switch     | 4.0.1            | tcp        
f41efe28  | m20k's Switch     | 3.0.0            | usb 
```

## twib connect-tcp

Attempts to connect to a device over TCP by address and optional port number.

```
$ twib connect-tcp 10.0.0.218
```

## twib run

Runs an executable on the target console. If the `-a` flag is not used, the executable will be run in a "managed" sysmodule process that is invisible to the rest of the system. The executable format for managed processes is NRO. If the `-a` flag is used, the process will be launched as a library applet instead. If you want to run a typical homebrew application, you will need to use the `-a` flag. The executable format for applet processes is also NRO.

In the future, both managed and applet processes will be able to be launched from ExeFS NSP, NSO, NRO, or ELF.

Twib will stay alive until the process exits. If the application implements Twili stdio, any output from the process will come out of twib and any input given to Twib will be sent to the target process. You can use the `-q` flag to silence the PID output if you are using twib in a shell script.

```
$ twib run test_helloworld.nro
PID: 0x85
Hello, World!
```

## twib reboot

Reboots the target console. This performs a hard, dirty reboot.

```
$ twib reboot
```

## twib coredump

Creates an ELF core dump from the given running process ID. This core dump will be suitable for loading in radare2, `aarch64-linux-gnu-gdb` (note: not `aarch64-none-elf-gdb`), IDA, or [faron](https://github.com/misson20000/CircuitBreaker). This may take a few minutes depending on how much memory the process is using, up to tens of minutes for games such as Breath of the Wild. The process is not terminated afterwards.

```
$ twib coredump am.elf 0x57
```

## twib terminate

Terminates a process on the target console by PID.

```
$ twib terminate 0x57
```

## twib ps

Lists all processes running on the target console, whether pm knows about them or not. One process will come up with error `0x16ef` in the result column. This process is Twili itself, and the error represents Twili's refusal to attempt to debug itself.

```
$ twib ps
Process ID | Result | Title ID          | Process Name | MMU Flags
0x1        | 0x0    | 0x100000000000000 | FS           | 0x27     
0x2        | 0x0    | 0x100000000000001 | Loader       | 0x27     
0x3        | 0x0    | 0x100000000000002 | NCM          | 0x27     
0x4        | 0x0    | 0x100000000000003 | ProcessMana  | 0x27     
0x5        | 0x0    | 0x100000000000004 | sm           | 0x27     
0x6        | 0x0    | 0x100000000000028 | spl          | 0x27     
0x51       | 0x0    | 0x100000000000021 | psc          | 0x137    
0x52       | 0x0    | 0x10000000000001d | pcie.withou  | 0x137   
...
```

## twib identify

Shows identifying information for the target console.

```
$ twib identify
bluetooth_bd_address : [ ... ]
device_nickname : m20k's Switch
firmware_version : [ ... ]
mii_author_id : [ ... ]
protocol : 2
serial_number : XAW...
service : twili
wireless_lan_mac_address : [ ... ]
```

## twib list-named-pipes

Lists any named pipes that exist on the target console. Typically, none will exist to be listed here. Named pipes are a way for sysmodules to output debug messages over Twili without being monitored by Twili.

```
$ twib list-named-pipes
```

## twib open-named-pipe

Opens a named pipe. Twib will stay alive until the pipe is closed, and will continuously output messages sent over the pipe.

```
$ twib open-named-pipe testpipe
```

## twib get-memory-info

Shows memory usage information on the target console.

```
$ twib get-memory-info
Twili Memory: 16 MiB / 38 MiB (42%)
System Category Limit: 280 MiB / 303 MiB (92%)
Application Category Limit: 0 MiB / 3285 MiB (0%)
Applet Category Limit: 82 MiB / 501 MiB (16%)
```

# Developer Details

## Project Organization

- **common**: Code common between Twili and Twib.
- **docs**: Documentation
- **hbabi_shim**: Homebrew ABI shim
- **twib**: PC-side bridge client
  - **cmake**: CMake modules
  - **externals**: Dependency submodules
  - **twib**: `twib` command-line tool
  - **twibd**: `twibd` daemon
- **twili**: Main `twili` sysmodule
  - **twili/bridge**: Bridge code
    - **twili/bridge/interfaces**: Twib endpoints
    - **twili/bridge/tcp**: TCPBridge implementation
    - **twili/bridge/usb**: USBBridge implementation
  - **twili/ipcbind**: IPC client code for various services
  - **twili/msgpack11**: msgpack11 submodule
  - **twili/process**: Process management code.
    - **twili/process/fs**: Virtual filesystem code
  - **twili/service**: IPC service code
- **twili_applet_shim**: Applet shim code
- **twili_common**: Code common between twili components that run on console

## Process Management

- **Process**: Base class. Exposes `GetPid`, `AddNotes`, and `Terminate`.
  - **MonitoredProcess**: Process started by Twili.
    - **ManagedProcess**: Processes that hide from `pm` and are managed by Twili.
    - **AppletProcess**: Library applet managed by am/pm.
  - **UnmonitoredProcess**: Shim for quick operations on arbitrary processes.

## Homebrew ABI Shim

This is a small chunk of code that is loaded before each application that obtains Homebrew ABI keys and handles from Twili, passes them on to the application, and passes the application's return code back to Twili when it exits.

## Applet Shim

Twili uses Atmosphere-loader's [SetExternalContentSource](https://github.com/Atmosphere-NX/Atmosphere/pull/225) extension to override the album applet with `twili_applet_shim`, which has two jobs. One of them is to connect to Twili and wait for messages. Only one instance of `twili_applet_shim` should be running in this mode at a time, and it is called the "control applet". When Twili needs to launch an applet, it uses the SetExternalContentSource extension again to add the target executable to the next instance of `twili_applet_shim` to be launched. It then sends a message to the control applet telling it to ask am to launch another album applet. This new instance of `twili_applet_shim` will come up and connect to Twili, and fulfill the other job, becoming a "host applet". Twili will attach this applet to the respective `AppletProcess` instance, and then pass over all relevant HBABI keys and the host applet shim will jump to the target process image.

## Twib

Twib is the workstation-side tool, consisting of two parts: `twibd`, and `twib`. `twibd` is the server side, which communicates with Twili over USB or sockets, and multiplexes access between multiple instances of the `twib` client-side. The `twibd` server-side has a number of backends, which handle talking to Twili. It also has a number of frontends, which accept connections from the `twib` client. On UNIX systems, this is a UNIX socket. On Windows systems, this is a named pipe.

![Twib block diagram](docs/twib_diagram.svg)

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

Twibd also implements device id 0, whose object id 0 represents `ITwibMetaInterface`.

### ITwibMetaInferface

#### Command ID 10: `LIST_DEVICES`

Takes an empty request payload, returns a MessagePack-encoded array of information about currently connected devices.

##### Response

MessagePack array of objects containing these keys:

- `device_id` - Device ID for message headers.
- `bridge_type` - String, either `usb` or `tcp` to indicate how the device is connected.
- `identification` - Identification message from `ITwibDeviceInterface#IDENTIFY`.

#### Command ID 11: `CONNECT_TCP`

Requests that the Twibd server attempt to connect to the device at the specified address over TCP. Empty response.

##### Request

```
u64 hostname_length;
u64 port_length;
char hostname[hostname_length];
char port[port_length];
```

### ITwibDeviceInterface

#### Command ID 10: `CREATE_MONITORED_PROCESS`

Takes a raw string indicating the type of monitored process to create (either `managed` or `applet`) and returns an [ITwibProcessMonitor](#ITwibProcessMonitor).

##### Request
```
char process_type[];
```

##### Response
```
u32 monitor_object_index; // ITwibProcessMonitor
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

#### Command ID 19: `OPEN_ACTIVE_DEBUGGER`

Opens a [debugger](#ITwibDebugger) for the given process.

##### Request
```
u64 process_id;
```

#### Response
```
u32 debugger_object_index; // ITwibDebugger
```

#### Command ID 20: `GET_MEMORY_INFO`

Takes empty request paylolad, responds with MessagePack-encoded information about current memory limits.

##### Response

MessagePack object containing these keys:

- `total_memory_available` - Total memory available to the Twili service process.
- `total_memory_usage` - Total memory usage by the Twili service process.
- `limits` - Array of details on global resource limits.
  - `category` - Resource limit category (0: `System`, 1: `Application`, 2: `Applet`)
  - `current_value` - Current memory usage within this category.
  - `limit_value` - Maximum allowed memory usage within this category.

### ITwibPipeReader

#### Command ID 10: `READ`

Reads data from the pipe, or blocks until some is available. Responds with the data that was read, or `TWILI_ERR_EOF` if the pipe is closed.

### ITwibPipeWriter

#### Command ID 11: `WRITE`

Writes data from the request payload to the pipe, blocking until it is read. Reponds with empty response, or `TWILI_ERR_EOF` if the pipe is closed.

#### Command ID 12: `CLOSE`

Closes the pipe. Empty request, empty response.

### ITwibDebugger

#### Command ID 10: `QUERY_MEMORY`

#### Command ID 11: `READ_MEMORY`

#### Command ID 12: `WRITE_MEMORY`

#### Command ID 13: `LIST_THREADS`

#### Command ID 14: `GET_DEBUG_EVENT`

#### Command ID 15: `GET_THREAD_CONTEXT`

#### Command ID 16: `BREAK_PROCESS`

#### Command ID 17: `CONTINUE_DEBUG_EVENT`

#### Command ID 18: `SET_THREAD_CONTEXT`

#### Command ID 19: `GET_NSO_INFOS`

#### Command ID 20: `WAIT_EVENT`

### ITwibProcessMonitor

#### Command ID 10: `LAUNCH`

#### Command ID 11: `RUN`

Reserved for future separation between process creation and process starting, to aid debugging.

#### Command ID 12: `TERMINATE`

#### Command ID 13: `APPEND_CODE`

#### Command ID 14: `OPEN_STDIN`

#### Command ID 15: `OPEN_STDOUT`

#### Command ID 16: `OPEN_STDERR`

#### Command ID 17: `WAIT_STATE_CHANGE`

# Other Tools

- [CircuitBreaker](https://github.com/misson20000/CircuitBreaker), an interactive hacking toolkit has two backends designed to work with Twili. The `faron` backend allows low-level inspection of core dumps generated by Twili. The `lanayru` backend allows you to attach CircuitBreaker to an active process running on the console.
