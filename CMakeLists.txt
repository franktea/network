cmake_minimum_required(VERSION 3.20)

project(network)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-O2 -DASIO_STANDALONE")

include(FetchContent)
FetchContent_Declare(asio
  GIT_REPOSITORY    https://github.com/chriskohlhoff/asio.git
  GIT_TAG master
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
)
FetchContent_Populate(asio)
include_directories(${asio_SOURCE_DIR}/asio/include/)

include(FetchContent)
FetchContent_Declare(
        ut
        GIT_REPOSITORY https://github.com/boost-ext/ut.git
        GIT_TAG        v1.1.8
)
FetchContent_MakeAvailable(ut)

set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include/)
set(ut_SINGLE_INCLUDE ${ut_SOURCE_DIR}/include) # for unit test
set(BINARY_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/bin)
#set(AWAITABLE_DEFAULT_H_DIR ${PROJECT_SOURCE_DIR}) # for awaitable_default.h

# include use_awaitable_default.h
include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(async_do)
add_subdirectory(timer-performance)
add_subdirectory(httpclient)
#add_subdirectory(httpclient-https)
add_subdirectory(socks5-daemon)
add_subdirectory(redis-asio)
add_subdirectory(fixed-size-package)
add_subdirectory(io_context)
add_subdirectory(simplest_httpsvr)
add_subdirectory(custom_errorcode)
add_subdirectory(custom_timer_manager)
add_subdirectory(coro_udp_client_server)
add_subdirectory(coro_echo_client_server)
add_subdirectory(executor)
add_subdirectory(timer_usage)
add_subdirectory(thread_switch)
add_subdirectory(use_awaitable_as_default)
add_subdirectory(quicksort_coro)
add_subdirectory(composed_operations)
add_subdirectory(any_completion_handler)

