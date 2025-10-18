project(timer_performance)

add_executable(tp main.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(tp PRIVATE ${ASIO_INCLUDE_DIR})
if(NOT WIN32)
    target_link_libraries(tp pthread)
endif()
