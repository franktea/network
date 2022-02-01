project(socks5-daemon)

add_executable(socks5d sock5d.cpp)
set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})
target_include_directories(socks5d PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(socks5d pthread)
