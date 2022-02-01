cmake_minimum_required(VERSION 3.20)

project(network)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-O2 -DASIO_STANDALONE -Werror")

include(FetchContent)
FetchContent_Declare(asio
  GIT_REPOSITORY    https://github.com/chriskohlhoff/asio.git
  GIT_TAG master
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
)
FetchContent_Populate(asio)
include_directories(${asio_SOURCE_DIR}/asio/include/)

set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include/)
set(CATCH_SINGLE_INCLUDE ${PROJECT_SOURCE_DIR}/3rdparty/unit_test) # for unit test
set(BINARY_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(timer-performance)
add_subdirectory(httpclient)
#add_subdirectory(httpclient-https)
add_subdirectory(socks5-daemon)
add_subdirectory(redis-asio)
add_subdirectory(fixed-size-package)
add_subdirectory(io_context)
add_subdirectory(simplest_httpsvr)
add_subdirectory(custom_errorcode)
#add_subdirectory(custom_timer_manager)
add_subdirectory(async_udp_client)
add_subdirectory(executor)
add_subdirectory(timer_usage)

