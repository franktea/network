project(fixed-size-pkg)

add_executable(fixed-size-server server.cpp)
add_executable(fixed-size-client client.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(fixed-size-server PRIVATE ${ASIO_INCLUDE_DIR})
if(NOT WIN32)
    target_link_libraries(fixed-size-server pthread)
endif()

target_include_directories(fixed-size-client PRIVATE ${ASIO_INCLUDE_DIR})
if(NOT WIN32)
    target_link_libraries(fixed-size-client pthread)
endif()
