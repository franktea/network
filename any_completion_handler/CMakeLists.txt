project(any_completion_handler)

add_executable(async_do_run do_run_impl.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_link_libraries(async_do_run pthread)
