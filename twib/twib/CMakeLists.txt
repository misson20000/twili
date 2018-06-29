set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_executable(twib Twib.cpp Messages.cpp RemoteObject.cpp ITwibMetaInterface.cpp ITwibDeviceInterface.cpp ../../util.cpp)

target_link_libraries(twib twib-log twib-buffer)

include_directories(msgpack11 INTERFACE)
target_link_libraries(twib msgpack11)

include_directories(CLI11 INTERFACE)
target_link_libraries(twib CLI11)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(twib Threads::Threads)
