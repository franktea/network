project(redis-assio)

add_executable(mt-redis-cli multi-thread-client.cpp response_parser.cpp)
add_executable(general-redis-cli general-client.cpp response_parser.cpp)
add_executable(protocol_parser_test response_parser.cpp response_parser_test.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(mt-redis-cli PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(mt-redis-cli ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(general-redis-cli PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(general-redis-cli ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(protocol_parser_test PRIVATE ${ASIO_INCLUDE_DIR} ${CATCH_SINGLE_INCLUDE})
target_link_libraries(protocol_parser_test ${CMAKE_THREAD_LIBS_INIT})