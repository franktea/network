project(io_context_usage)

add_executable(io_context_usage io_context_usage.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(io_context_usage PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(io_context_usage pthread)