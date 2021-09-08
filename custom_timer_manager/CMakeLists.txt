project(custom_timer_mgr)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(custom_timer_mgr custom_timer_mgr.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(custom_timer_mgr PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(custom_timer_mgr Threads::Threads)