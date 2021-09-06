project(async_udp_client)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(async_udp_client async_udp_client_test.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(async_udp_client PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(async_udp_client Threads::Threads)