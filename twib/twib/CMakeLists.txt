add_executable(twib Twib.cpp Messages.cpp RemoteObject.cpp ITwibMetaInterface.cpp ITwibDeviceInterface.cpp ../../util.cpp)

target_link_libraries(twib twib-log twib-buffer)

include_directories(msgpack11 INTERFACE)
target_link_libraries(twib msgpack11)

include_directories(CLI11 INTERFACE)
target_link_libraries(twib CLI11)
