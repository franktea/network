project(custom_errorcode)

add_executable(custom_errorcode custom-error-code.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(custom_errorcode PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(custom_errorcode pthread)