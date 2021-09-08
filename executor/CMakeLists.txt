project(executor)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(executor_basic basic_usage.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(executor_basic PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(executor_basic Threads::Threads)