set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCE Twib.cpp Client.cpp SocketClient.cpp Messages.cpp RemoteObject.cpp msgpack_show.cpp interfaces/ITwibMetaInterface.cpp interfaces/ITwibDeviceInterface.cpp interfaces/ITwibPipeReader.cpp interfaces/ITwibPipeWriter.cpp interfaces/ITwibProcessMonitor.cpp interfaces/ITwibDebugger.cpp interfaces/ITwibFilesystemAccessor.cpp interfaces/ITwibFileAccessor.cpp)

if(TWIB_NAMED_PIPE_FRONTEND_ENABLED)
	set(SOURCE ${SOURCE} NamedPipeClient.cpp)
endif()

if(TWIB_GDB_ENABLED)
	set(SOURCE ${SOURCE} GdbConnection.cpp GdbStub.cpp)
endif()

add_executable(twib ${SOURCE})

target_link_libraries(twib twib-platform twib-common)

include_directories(msgpack11 INTERFACE)
target_link_libraries(twib msgpack11)

include_directories(CLI11 INTERFACE)
target_link_libraries(twib CLI11)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(twib Threads::Threads)

if (WIN32)
	target_link_libraries(twib wsock32 ws2_32)
endif()

install(TARGETS twib RUNTIME DESTINATION bin)
