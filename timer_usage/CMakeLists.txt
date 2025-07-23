project(timer_usage)

add_executable(timer_basic basic_usage.cpp)
add_executable(timer_repeated repeated.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(timer_basic PRIVATE ${ASIO_INCLUDE_DIR})
if(NOT WIN32)
    target_link_libraries(timer_basic pthread)
endif()

target_include_directories(timer_repeated PRIVATE ${ASIO_INCLUDE_DIR})
if(NOT WIN32)
    target_link_libraries(timer_repeated pthread)
endif()
