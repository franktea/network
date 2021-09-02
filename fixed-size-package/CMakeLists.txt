project(fixed-size-pkg)

add_executable(fixed-size-server server.cpp)
add_executable(fixed-size-client client.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(fixed-size-server PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(fixed-size-server ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(fixed-size-client PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(fixed-size-client ${CMAKE_THREAD_LIBS_INIT})