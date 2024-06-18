cmake_minimum_required(VERSION 3.20)

project(protobuf-rpc)

set(CMAKE_CXX_STANDARD 20)

find_package(absl REQUIRED )
include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})

include(FetchContent)
FetchContent_Declare(asio
  GIT_REPOSITORY    https://github.com/chriskohlhoff/asio.git
  GIT_TAG master
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
)
FetchContent_Populate(asio)
include_directories(${asio_SOURCE_DIR}/asio/include/)

add_executable(calc-server calc.pb.cc pbrpc.cpp rpc_pkg.pb.cc calc-server.cpp)
add_executable(calc-client calc.pb.cc pbrpc.cpp rpc_pkg.pb.cc calc-client.cpp)

target_link_libraries(calc-server
    ${PROTOBUF_LIBRARY}
    absl::log_internal_message
    absl::log_internal_check_op
)

target_link_libraries(calc-client
    ${PROTOBUF_LIBRARY}
    absl::log_internal_message
    absl::log_internal_check_op
)
