project(use_awaitable_as_default)

add_executable(use_awaitable_always use_awaitable_always.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(tp PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(tp pthread)