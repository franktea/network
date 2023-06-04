project(key_value_cache)

add_executable(key_value_cache key_value_cache.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_link_libraries(key_value_cache pthread)
