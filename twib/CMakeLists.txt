cmake_minimum_required(VERSION 3.1)
project(twib)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (WIN32)
	add_definitions(-DWIN32_LEAN_AND_MEAN)
	if(MINGW)
		set (CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -static -static-libgcc -static-libstdc++")
	endif()
endif()

# enable ASAN
if(NOT WIN32)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif()

set(MSGPACK11_BUILD_TESTS OFF CACHE BOOL "Build msgpack11 unit tests")

set(WITH_SYSTEMD OFF CACHE BOOL "Enable systemd integration in twibd")
set(TWIBD_HOTPLUG_ENABLED ON CACHE BOOL "Enable USB hotplug in twibd")

if(NOT WIN32)
	set(TWIB_UNIX_FRONTEND_ENABLED ON CACHE BOOL "Enable UNIX socket frontend")
else()
	set(TWIB_UNIX_FRONTEND_ENABLED OFF CACHE BOOL "Enable UNIX socket frontend")
endif()
set(TWIB_UNIX_FRONTEND_DEFAULT_PATH "/var/run/twibd.sock" CACHE FILEPATH "Default path for the twibd UNIX socket frontend")

set(TWIB_TCP_FRONTEND_ENABLED ON CACHE BOOL "Enable TCP socket frontend")
set(TWIB_TCP_FRONTEND_DEFAULT_PORT 15151 CACHE STRING "Default port for twibd TCP socket frontend")

set(TWILI_VENDOR_ID 0x1209 CACHE STRING "Vendor ID for Twili USB device")
set(TWILI_PRODUCT_ID 0x8b00 CACHE STRING "Product ID for Twili USB device")

set(TWIBD_ACCEPT_NINTENDO_SDK_DEBUGGER ON CACHE BOOL "Allow twibd to accept Nintendo SDK debugger VID/PID as a Twili device (for 1.0.0)")
set(TWIBD_NINTENDO_SDK_DEBUGGER_VENDOR_ID 0x057e CACHE STRING "Vendor ID for Nintendo SDK debugger")
set(TWIBD_NINTENDO_SDK_DEBUGGER_PRODUCT_ID 0x3000 CACHE STRING "Product ID for Nintendo SDK debugger")

message(STATUS "systemd support: ${WITH_SYSTEMD}")
message(STATUS "twibd hotplug: ${TWIBD_HOTPLUG_ENABLED}")
message(STATUS "twib unix frontend enabled: ${TWIB_UNIX_FRONTEND_ENABLED}")
message(STATUS "twib unix frontend default path: ${TWIB_UNIX_FRONTEND_DEFAULT_PATH}")
message(STATUS "twib tcp frontend enabled: ${TWIB_TCP_FRONTEND_ENABLED}")
message(STATUS "twib tcp frontend default port: ${TWIB_TCP_FRONTEND_DEFAULT_PORT}")
message(STATUS "twili vendor id: ${TWILI_VENDOR_ID}")
message(STATUS "twili product id: ${TWILI_PRODUCT_ID}")
message(STATUS "twibd accept nintendo sdk debugger: ${TWIBD_ACCEPT_NINTENDO_SDK_DEBUGGER}")
message(STATUS "twibd nintendo sdk debugger vendor id: ${TWIBD_NINTENDO_SDK_DEBUGGER_VENDOR_ID}")
message(STATUS "twibd nintendo sdk debugger product id: ${TWIBD_NINTENDO_SDK_DEBUGGER_PRODUCT_ID}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

configure_file(
	"${PROJECT_SOURCE_DIR}/config.hpp.in"
	"${PROJECT_BINARY_DIR}/config.hpp"
	)

add_definitions("-DPROJECT_ROOT=\"${PROJECT_SOURCE_DIR}\"") # for logging

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/../common/")

add_library(twib-common Logger.cpp ../common/Buffer.cpp ../common/util.cpp ResultError.cpp MessageConnection.cpp SocketMessageConnection.cpp)
add_subdirectory(externals EXCLUDE_FROM_ALL)
add_subdirectory(twibd)
add_subdirectory(twib)
