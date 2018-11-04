set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCE Twibd.cpp Messages.cpp LocalClient.cpp SocketFrontend.cpp BridgeObject.cpp)
if(TWIB_NAMED_PIPE_FRONTEND_ENABLED)
	set(SOURCE ${SOURCE} NamedPipeFrontend.cpp)
endif()
if(TWIBD_TCP_BACKEND_ENABLED)
	set(SOURCE ${SOURCE} TCPBackend.cpp)
endif()
if(TWIBD_LIBUSB_BACKEND_ENABLED)
	set(SOURCE ${SOURCE} USBBackend.cpp)
endif()
if(TWIBD_LIBUSBK_BACKEND_ENABLED)
	set(SOURCE ${SOURCE} USBKBackend.cpp)
endif()
add_executable(twibd ${SOURCE})

target_link_libraries(twibd twib-common)

include_directories(msgpack11 INTERFACE)
target_link_libraries(twibd msgpack11)

include_directories(CLI11 INTERFACE)
target_link_libraries(twibd CLI11)

if(TWIBD_LIBUSB_BACKEND_ENABLED)
	find_package(libusb-1.0 REQUIRED)
	include_directories(${LIBUSB_1_INCLUDE_DIRS} INTERFACE)
	target_link_libraries(twibd ${LIBUSB_1_LIBRARIES})
endif()

if(TWIBD_LIBUSBK_BACKEND_ENABLED)
	find_package(libusbK REQUIRED)
	include_directories(${LIBUSBK_INCLUDE_DIRS} INTERFACE)
	target_link_libraries(twibd ${LIBUSBK_LIBRARIES})

	find_package(SetupAPI REQUIRED)
	target_link_libraries(twibd ${SETUPAPI_LIBRARIES})
endif()

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
