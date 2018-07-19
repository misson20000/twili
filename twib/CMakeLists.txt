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

set(Twibd_UNIX_SOCKET_PATH "./twibd.sock")
set(Twibd_HOTPLUG_ENABLED true)
set(Twili_VENDOR_ID 0x1209)
set(Twili_PRODUCT_ID 0x8b00)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

configure_file(
	"${PROJECT_SOURCE_DIR}/config.hpp.in"
	"${PROJECT_BINARY_DIR}/config.hpp"
	)

# Provide __FILE_SHORT__ for logging
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILE_SHORT__='\"placeholder\"'")

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/../")

add_library(twib-log Logger.cpp)
add_library(twib-buffer Buffer.cpp)
add_subdirectory(externals)
add_subdirectory(twibd)
add_subdirectory(twib)
