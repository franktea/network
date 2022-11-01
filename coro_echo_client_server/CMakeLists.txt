project(coro_echo_client_server)

add_executable(tcp_echo_client_server tcp_echo_client_server.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

#target_include_directories(tcp_echo_client_server PRIVATE ${ASIO_INCLUDE_DIR} ${AWAITABLE_DEFAULT_H_DIR})
target_link_libraries(tcp_echo_client_server pthread)