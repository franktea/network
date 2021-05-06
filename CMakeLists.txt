cmake_minimum_required(VERSION 3.19)

project(network)

set(CMAKE_CXX_FLAGS "-g -O2 -std=c++11 -DASIO_STANDALONE -Werror")

set(ASIO_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/asio/asio/include)
set(CATCH_SINGLE_INCLUDE ${PROJECT_SOURCE_DIR}/3rdparty/unit_test) # for unit test
set(BINARY_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(timer-performance)
add_subdirectory(httpclient)
#add_subdirectory(httpclient-https)
add_subdirectory(redis-asio)
add_subdirectory(fixed-size-package)
add_subdirectory(io_context)
add_subdirectory(simplest_httpsvr)
add_subdirectory(custom_errorcode)
add_subdirectory(custom_timer_manager)
add_subdirectory(async_udp_client)
add_subdirectory(executor)
add_subdirectory(timer_usage)

