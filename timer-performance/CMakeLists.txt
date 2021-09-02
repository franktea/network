project(timer_performance)

add_executable(tp main.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(tp PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(tp ${CMAKE_THREAD_LIBS_INIT})