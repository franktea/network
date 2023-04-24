project(composed_operations)

add_executable(echo_client_composed echo_client_composed.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

#target_include_directories(tcp_echo_client_server PRIVATE ${ASIO_INCLUDE_DIR} ${AWAITABLE_DEFAULT_H_DIR})
target_link_libraries(echo_client_composed pthread)