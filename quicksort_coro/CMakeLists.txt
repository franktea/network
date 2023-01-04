project(quicksort_coro)

add_executable(quicksort_coro quicksort_coro.cpp)
set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})
target_include_directories(quicksort_coro PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(quicksort_coro pthread)
