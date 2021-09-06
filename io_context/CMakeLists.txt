project(io_context_usage)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(io_context_usage io_context_usage.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(io_context_usage PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(io_context_usage Threads::Threads)