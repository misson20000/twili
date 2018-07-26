set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCE Twibd.cpp USBBackend.cpp Messages.cpp LocalClient.cpp SocketFrontend.cpp)
add_executable(twibd ${SOURCE})

target_link_libraries(twibd twib-log twib-buffer)

include_directories(msgpack11 INTERFACE)
target_link_libraries(twibd msgpack11)

include_directories(CLI11 INTERFACE)
target_link_libraries(twibd CLI11)

find_package(libusb-1.0 REQUIRED)
include_directories(${LIBUSB_1_INCLUDE_DIRS})
target_link_libraries(twibd ${LIBUSB_1_LIBRARIES})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(twibd Threads::Threads)

if (WIN32)
	target_link_libraries(twibd wsock32 ws2_32)
endif()

if(WITH_SYSTEMD)
	find_package(systemd REQUIRED)
	include_directories(${SYSTEMD_INCLUDE_DIRS})
	target_link_libraries(twibd ${SYSTEMD_LIBRARIES})

	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/twibd.socket"
		"${CMAKE_CURRENT_BINARY_DIR}/twibd.socket"
		)
	
	set(SYSTEMD_CONFIG_DIR "lib/systemd/system")
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/twibd.socket DESTINATION ${SYSTEMD_CONFIG_DIR})
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/twibd.service DESTINATION ${SYSTEMD_CONFIG_DIR})
endif()

install(TARGETS twibd RUNTIME DESTINATION bin)
