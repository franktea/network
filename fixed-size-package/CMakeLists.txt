project(fixed-size-pkg)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(fixed-size-server server.cpp)
add_executable(fixed-size-client client.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(fixed-size-server PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(fixed-size-server Threads::Threads)

target_include_directories(fixed-size-client PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(fixed-size-client Threads::Threads)