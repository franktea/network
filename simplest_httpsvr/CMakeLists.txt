project(simplest-httpsvr)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(simplest-httpsvr http_svr.cpp)
set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})
target_include_directories(simplest-httpsvr PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(simplest-httpsvr Threads::Threads)
