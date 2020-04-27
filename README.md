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
  * [Application Debugging](#application-debugging)
- [Installation](#installation)
  * [Linux](#linux)
    + [Arch Linux](#arch-linux)
    + [Other Linux Distrubutions](#other-linux-distrubutions)
  * [OSX](#osx)
  * [Windows](#windows)
- [Configuration](#configuration)
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
  * [twib debug](#twib-debug)
  * [twib launch](#twib-launch)
  * [twib pull](#twib-pull)
  * [twib push](#twib-push)
- [Developer Details](#developer-details)
  * [Project Organization](#project-organization)
  * [Title Table](#title-table)
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
6.1.0 | Tested working
6.2.0 | Tested working
7.0.0 | Expected working
7.0.1 | Tested working
8.0.0 | Tested working

## Limitations

These are temporary and are planned to be addressed.

- Applets cannot be launched without an active control applet (hbmenu open).
- Applets are library applets, not applications (this means they're limited to using memory in the APPLET pool).

## Application Development

If you're developing with libtransistor, Twili integration is automatic. Standard I/O will automatically be attached to the twib console.

If you're developing an application with libnx, try [libtwili](https://github.com/misson20000/twili-libnx) for twib stdio.
Most libnx applications are applets. If you're developing an applet (interacts with the user), make sure you use `-a` with `twib run` to run it as an applet.
If you're developing a sysmodule, you can use `twib run` without `-a` to run it as a sysmodule.

## Application Debugging

As of v1.2.0, twib includes a GDB stub.

### Features

- Works with homebrew apps and sysmodules as well as official Nintendo software
- Breakpoints
- Continuing from breakpoints and single-stepping (with twili-gdb)
- Memory viewing (`p` command)
- Function calling (`p` command with twili-gdb)
- Multiprocess extensions

### Limitations

- **[twili-gdb](https://github.com/misson20000/twili-gdb) is required for single-stepping.** This is because the Horizon kernel does not implement hardware single-stepping, so I had to implement software single stepping in GDB.
- **[twili-gdb](https://github.com/misson20000/twili-gdb) is required for function calling.** This is because Horizon executables are marked as shared libraries but have entry point 0x0, which GDB interprets as no entry point.
- **GDB File I/O and `run` commands are unsupported.** Not yet implemented.
- **Hardware breakpoints and watchpoints are unsupported.** Not yet implemented.
- **Windows is unsupported.** Not yet implemented.

### Installing twili-gdb

If you are running Arch Linux, there is [an AUR package](https://aur.archlinux.org/packages/gdb-twili-git/) for twili-gdb. Otherwise, you will need to build it from source. It's a standard autotools build. Make sure to specify `--target=twili`, or, if you prefer longer prefixes, `aarch64-twili-elf` should work too.

```
$ git clone https://github.com/misson20000/twili-gdb.git
$ cd twili-gdb
$ mkdir build
$ cd build
$ ../configure --target=twili --enable-targets=twili --prefix=/usr --disable-sim
$ make
$ sudo make install
```

### Using the GDB Stub

Use the gdb stub with `target extended-remote | twib gdb`.

```
$ twili-gdb
GNU gdb (GDB) 8.3.50.20190328-git
Copyright (C) 2019 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "--host=x86_64-pc-linux-gnu --target=twili".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word".
(gdb) target extended-remote | twib gdb
Remote debugging using | twib gdb
(gdb)
```

You can attach to an existing process with `attach 0x<pid>`. Use `twib ps` from another terminal to list process IDs. I recommend loading symbol files (via command-line or via `file` command) before attaching processes, since the stub does not provide symbols.

Use `info threads` to list threads, `break` to set breakpoints, `c` to continue, etc.

#### Suspended Start

Many times, you don't want to attach to an existing process, but you want to debug a process from the start. There are two ways to do this.

##### Monitored Process Suspended Start

For monitored processes (`twib run`), you can pass the `-d` (or `--debug-suspend`) option. Twili will block HBABIShim/AppletHost on an IPC call until a debugger is attached. This is usually good enough, but if you need suspended start on an unmonitored process (official processes) or need to debug before AppletHost, there is another way.

##### PM Suspended Start

The GDB stub implements two monitor commands: `wait application` and `wait title 0x<title id>`. These commands will wait for either any application to be launched, or the specified title ID respectively. When the application or title is launched, it will be suspended before any code at all even executes and the PID will be printed in the GDB console. Use the `attach` command to attach to the suspended process.

```
(gdb) monitor wait application
PID: 0x8a
(gdb) attach 0x8a
```

### Details

#### Base Address Detection

For "monitored processes" (launched from twib), Twili reports the base address to gdb as wherever the user-supplied executable was placed in memory. This is usually what you want, but not that this skips over HBABIShim or AppletHost, which you may see in your backtrace. For non-monitored processes, the base address reported to gdb is wherever the first static code module is. This is usually `rtld` or `main`.

#### Breakpoints

Although Horizon supports hardware breakpoints, I don't use them. Breakpoints are implemented as software breakpoints, which overwrite instructions in the process with trap instructions that trigger exceptions that gdb intercepts. Continuing from one of these requires restoring the original instruction, single stepping the thread one instruction, then restoring the trap instruction.

#### Single Stepping

Horizon does not support single stepping. Single stepping is implemented by predicting the next instruction and placing a breakpoint on it.

#### Debugging PID 0

On lower firmware versions, process IDs begin at 0. This causes assertion failures in GDB if you try to attach to process 0, so the stub will automatically translate PID 512 into process 0.

# Installation

Download the [latest release](https://github.com/misson20000/twili/releases/latest) of `twili.zip` and extract it to the root of your microSD card, letting the `atmosphere` directory merge with any existing directory tree. The only CFW that I officially support is Atmosphère (v0.8.3+). Running Twili on piracy firmware will make me cry, so don't do it. :cry:

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

# Configuration

The Twili sysmodule has a number of different configuration options which can be changed by modifying `sdmc:/twili.ini`. If the file does not exist, it will be generated from the current defaults.
The default configuration is as follows.

```
; Twili Configuration File
[twili]
service_name = twili
; paths are relative to root of sd card
hbmenu_path = /hbmenu.nro
temp_directory = /.twili_temp

[pipes]
pipe_buffer_size_limit = 0x80000

[logging]
verbosity = 0
enable_usb = true

[usb_bridge]
enabled = true

[tcp_bridge]
enabled = true
port = 15152
```

## `[twili]`

### `service_name`

Default: `twili`

Controls the name that is used to register the twili service with `sm`.
Changing this will break a lot of things if you're not careful, but may be useful for development.

### `hbmenu_path`

Default: `/hbmenu.nro`

Contains a path relative to the root of the SD card that should be used to access the homebrew menu NRO.

### `temp_directory`

Default: `/.twili_temp`

Contains a path relative to the root of the SD card that should be used as a temporary directory. This directory will be created if it does not exist, and all files inside it will be deleted on every boot. It is used for storing files sent over `twib run`.

## `[pipes]`

### `pipe_buffer_size_limit`

Default: `0x80000`

Twib pipes (used for standard i/o in `twib run`) will buffer up to a maximum of this amount of data. If more data is written, the write will block until the other end reads out data instead of completing instantly.

This buffering behavior greatly speeds up small writes, since otherwise each write would involve round trip USB transactions. However, it loses the assurance that a write will not return until the data has been delivered to the other end. If this is important, the `pipe_buffer_size_limit` can be set to zero (`0`) to disable buffering and block writes until the data is delivered to the other end and another write is ready.

## `[logging]`

### `verbosity`

Default: 0

Controls verbosity threshold for various log messages for debugging. Higher is more verbose.

- **0:** Default
- **1:** `USBBridge::RequestReader` shows message headers

### `enable_usb`

Default: `true`

Controls whether the USB serial port endpoints are exposed or not.

## `[usb_bridge]`

### `enabled`

Default: `true`

Controls whether the USB bridge is enabled or not. This can be turned off if it's conflicting with something else that needs USB, or if Nintendo changes the `usb:ds` interface again.

## `[tcp_bridge]`

### `enabled`

Default: `true`

Controls whether the TCP bridge is enabled or not. This can be turned off if you don't want Twili to constantly try to bring up a network connection, or if you're concerned about security.

### `port`

Default: `15152`

Controls which port the TCP bridge listens on.

# Building From Source

## Twili

First, you will need to install libtransistor. Download the [latest release](https://github.com/reswitched/libtransistor/releases/latest), extract it somewhere, and set `LIBTRANSISTOR_HOME` to that path. After that, building Twili should be as simple as running `make`. To use Atmosphère-boot2 to launch Twili, you can copy the files from `build/atmosphere/` to the `/atmosphere/` directory on your microSD card.

## Twib

### Linux / OSX

You will need libusb development headers installed. On Debian-based distros, these are usually in the `libusb-1.0.0-dev` package.

Clone this repo and clone its submodules.

```
$ git clone --recurse-submodules git@github.com:misson20000/twili.git
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

Finally, you can install twib/twibd.

```
$ sudo make install
```

If you are using systemd and built with systemd support, you will want to enable the twibd socket.

```
$ sudo systemctl enable --now twibd.socket
```

If you are using macOS/OSX and built with launchd support, you will want to enable the twibd service.

```
$ sudo launchctl enable system/com.misson20000.twibd
$ sudo launchctl bootstrap system /Library/LaunchDaemons/com.misson20000.twibd.plist
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
  -n,--pipe-name TEXT (ENV:TWIB_NAAMED_PIPE_FRONTEND_NAME)
                              Name for the twibd pipe

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
  debug                       Prints debug info
  launch                      Launches an installed title
  pull                        Pulls files from device's SD card
  push                        Pushes files to device's SD card
```

All `twib` commands require a device to be specified, except for `list-devices` and `connect-tcp`. If no device is explicitly specified and there is exactly one device currently connected to the daemon, that device will be used. Otherwise, a device must be specified by device ID (obtained from `list-devices`) via the `-d` option or the `TWIB_DEVICE` environment variable.

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

Runs an NRO executable on the target console.

If the `-a` flag is used, the executable will be run in applet mode. These are called AppletProcess internally and are managed by AppletTracker. If the homebrew menu is open already, it will be closed and the applet started. Otherwise, you will have to open the homebrew menu first and the applet will be automatically launched. Only one such applet can be run at a time, so they will be queued instead.

If the `-a` flag is not used, the executable will be run as a background process managed via `pm:shell`. These are called ShellProcess internally and are managed by ShellTracker.

In the future, both shell and applet processes will be able to be launched from ExeFS NSP, NSO, NRO, or ELF.

Twib will stay alive until the process exits. If the application implements Twili stdio, any output from the process will come out of twib and any input given to Twib will be sent to the target process. You can use the `-q` flag to silence the PID output if you are using twib in a shell script or pipeline.

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

## twib debug

Requests for Twili to print debug information to its standard output. Twib shouldn't output anything. See twibd's logs for output.

## twib launch

Launches a title on the console using pm:shell. Note that this is not sufficient to launch games.

```
$ twib launch 01006A800016E000 gc
0x81
```


### Understood Locations:
- none
- host
- gamecard, gc
- nand-system, system
- nand-user, user
- sdcard, sd

## twib pull

Pulls files from the device's SD card. Multiple files can be pulled if the destination is a directory.
If the destination is `-`, file contents will be written to standard output.

```
$ twib pull /path/to/file/on/device1 /another/file/on/device2 /destination/directory/on/host/
/path/to/file/on/device1 -> /destination/directory/on/host/device1
/another/file/on/device2 -> /destination/directory/on/host/device2
```

**NOTE:** If you're running `twib` from a mingw environment, you will need to put two forward slashes in front of device paths to prevent them from being [converted](http://www.mingw.org/wiki/Posix_path_conversion) to Windows paths.

```
$ twib pull //path/to/file/on/device1 //another/file/on/device2 /destination/directory/on/host/
/path/to/file/on/device1 -> /destination/directory/on/host/device1
/another/file/on/device2 -> /destination/directory/on/host/device2
```

## twib push

Pushes files to device's SD card. Multiple files can be pushed if the destination is a directory.

```
$ twib push /path/to/file/on/host1 /path/to/another/host/file /destination/directory/on/device/
/path/to/file/on/host1 -> /destination/directory/on/device/host1
/path/to/another/host/file -> /destination/directory/on/device/file
```

**NOTE:** If you're running `twib` from a mingw environment, you will need to put two forward slashes in front of device paths to prevent them from being [converted](http://www.mingw.org/wiki/Posix_path_conversion) to Windows paths.

```
$ twib push /path/to/file/on/host1 /path/to/another/host/file //destination/directory/on/device/
/path/to/file/on/host1 -> /destination/directory/on/device/host1
/path/to/another/host/file -> /destination/directory/on/device/file
```

# Developer Details

## Project Organization

- **common**: Code common between Twili and Twib.
- **docs**: Documentation
- **hbabi_shim**: Homebrew ABI shim
- **twib**: PC-side bridge client
  - **cmake**: CMake modules
  - **common**: Code common between tool and daemon
  - **daemon**: `twibd` driver daemon
  - **externals**: Dependency submodues
  - **platform**: Platform specific code
  - **tool**: `twib` command-line tool
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

## Title Table

| Title ID         | Description |
|------------------|---|
| 0100000000006480 | Twili Sysmodule |
| 0100000000006481 | Twili Managed Process (twib run) |
| 0100000000006482 | Twili Applet Shim |

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
