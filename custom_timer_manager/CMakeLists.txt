project(custom_timer_mgr)

add_executable(custom_timer_mgr custom_timer_mgr.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(custom_timer_mgr PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(custom_timer_mgr ${CMAKE_THREAD_LIBS_INIT})