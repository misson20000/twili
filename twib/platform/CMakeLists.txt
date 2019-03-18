set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COMMON_SOURCES)
set(PLATFORM_INCLUDES)

if(WIN32)
	set(PLATFORM_SOURCES windows/platform/platform.cpp windows/platform/EventLoop.cpp windows/platform/InputPump.cpp)
	set(PLATFORM_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/windows/")
else()
	set(PLATFORM_SOURCES unix/platform/platform.cpp unix/platform/EventLoop.cpp unix/platform/InputPump.cpp)
	set(PLATFORM_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/unix/")
endif()

add_library(twib-platform ${COMMON_SOURCE} ${PLATFORM_SOURCES})
target_link_libraries(twib-platform twib-common)
target_include_directories(twib-platform INTERFACE ${PLATFORM_INCLUDES})
