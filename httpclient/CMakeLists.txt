project(http_client)
cmake_minimum_required(VERSION 2.8)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

add_executable(${PROJECT_NAME} http_client.cpp AsyncHttpClient.cpp)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} pthread)
target_include_directories(${PROJECT_NAME} PUBLIC ${ASIO_INCLUDE_DIR})

