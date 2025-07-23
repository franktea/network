project(https_client)
cmake_minimum_required(VERSION 2.8)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

add_executable(${PROJECT_NAME} https-client.cpp AsyncHttpsClient.cpp)
if(NOT WIN32)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} pthread ssl crypto)
else()
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ssl crypto)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC ${ASIO_INCLUDE_DIR} /usr/local/Cellar/openssl/1.0.2o_1/include)

