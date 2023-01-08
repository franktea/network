project(thread_switch)

add_executable(thread_switch thread_switch.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_link_libraries(thread_switch pthread)
