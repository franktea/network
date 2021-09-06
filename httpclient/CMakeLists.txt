project(http_client)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

add_executable(${PROJECT_NAME} http_client.cpp AsyncHttpClient.cpp)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} Threads::Threads)
target_include_directories(${PROJECT_NAME} PUBLIC ${ASIO_INCLUDE_DIR})
