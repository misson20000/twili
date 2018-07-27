set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_executable(twib Twib.cpp Messages.cpp RemoteObject.cpp ITwibMetaInterface.cpp ITwibDeviceInterface.cpp ../../util.cpp msgpack_show.cpp ITwibPipeReader.cpp)

target_link_libraries(twib twib-log twib-buffer twib-error)

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
