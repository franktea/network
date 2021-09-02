project(executor)

add_executable(executor_basic basic_usage.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(executor_basic PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(executor_basic ${CMAKE_THREAD_LIBS_INIT})