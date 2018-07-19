set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCE Twibd.cpp USBBackend.cpp Messages.cpp LocalClient.cpp msgpack_show.cpp SocketFrontend.cpp)
add_executable(twibd ${SOURCE})

target_link_libraries(twibd twib-log twib-buffer)

include_directories(msgpack11 INTERFACE)
target_link_libraries(twibd msgpack11)

find_package(libusb-1.0 REQUIRED)
include_directories(${LIBUSB_1_INCLUDE_DIRS})
target_link_libraries(twibd ${LIBUSB_1_LIBRARIES})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(twibd Threads::Threads)

if (WIN32)
	target_link_libraries(twibd wsock32 ws2_32)
endif()
