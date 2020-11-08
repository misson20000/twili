set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"
	"${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
	)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(SOURCE Logger.cpp ../../common/err_defs.cpp ../../common/Buffer.cpp ../../common/util.cpp ResultError.cpp MessageConnection.cpp SocketMessageConnection.cpp Semaphore.cpp)

if(TWIB_NAMED_PIPE_FRONTEND_ENABLED)
	set(SOURCE ${SOURCE} NamedPipeMessageConnection.cpp)
endif()

add_library(twib-common ${SOURCE})
target_link_libraries(twib-common twib-platform)
