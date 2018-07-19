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

set(MSGPACK11_BUILD_TESTS OFF CACHE BOOL "foo")

set(WITH_SYSTEMD OFF CACHE BOOL "Enable systemd integration in twibd")
set(TWIBD_UNIX_SOCKET_PATH "/var/run/twibd.sock" CACHE FILEPATH "Path for the twibd unix socket")
set(TWIBD_HOTPLUG_ENABLED ON CACHE BOOL "Enable USB hotplug in twibd")
set(TWILI_VENDOR_ID 0x1209 CACHE STRING "Vendor ID for Twili USB device")
set(TWILI_PRODUCT_ID 0x8b00 CACHE STRING "Product ID for Twili USB device")

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
include_directories("${PROJECT_SOURCE_DIR}/../")

add_library(twib-log Logger.cpp)
add_library(twib-buffer Buffer.cpp)
add_subdirectory(externals EXCLUDE_FROM_ALL)
add_subdirectory(twibd)
add_subdirectory(twib)
