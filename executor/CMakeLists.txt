project(executor)

add_executable(timer_executor timer_executor.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(timer_executor PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(timer_executor pthread)
