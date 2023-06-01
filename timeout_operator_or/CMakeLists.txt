project(timeout_operator_or)

add_executable(timeout_operator_or timeout_operator_or.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_link_libraries(timeout_operator_or pthread)
