project(async_do)

add_executable(async_do async_do.cpp)
add_executable(async_wait_for async_wait_for.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_link_libraries(async_do pthread)
target_link_libraries(async_wait_for pthread)
