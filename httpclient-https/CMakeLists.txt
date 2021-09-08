project(https_client)
cmake_minimum_required(VERSION 3.1)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

add_executable(${PROJECT_NAME} https-client.cpp AsyncHttpsClient.cpp)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} Threads::Threads ssl crypto)
target_include_directories(${PROJECT_NAME} PUBLIC ${ASIO_INCLUDE_DIR} /usr/local/Cellar/openssl/1.0.2o_1/include)
