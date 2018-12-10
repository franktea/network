project(simplest-httpsvr)

add_executable(simplest-httpsvr http_svr.cpp)
set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})
target_include_directories(simplest-httpsvr PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(simplest-httpsvr pthread)
