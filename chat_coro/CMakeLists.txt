project(chat_coro)

add_executable(chat_server_coro chat_server_coro.cpp)
add_executable(chat_client_coro chat_client_coro.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(chat_server_coro PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(fixed-size-server pthread)

target_include_directories(chat_client_coro PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(chat_client_coro pthread)
