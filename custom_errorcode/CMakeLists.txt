project(custom_errorcode)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(custom_errorcode custom-error-code.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(custom_errorcode PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(custom_errorcode Threads::Threads)