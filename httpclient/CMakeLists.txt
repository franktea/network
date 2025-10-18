project(http_client)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

add_executable(${PROJECT_NAME} http_client.cpp AsyncHttpClient.cpp)
if(NOT WIN32)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} pthread)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${ASIO_INCLUDE_DIR})


