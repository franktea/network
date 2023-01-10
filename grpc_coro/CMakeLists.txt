cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-Wno-deprecated-declarations)

project(grpc_coro)

include(FetchContent)

FetchContent_Declare(asio
  GIT_REPOSITORY    https://github.com/chriskohlhoff/asio.git
  GIT_TAG master
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
)
FetchContent_Populate(asio)
include_directories(${asio_SOURCE_DIR}/asio/include/)

FetchContent_Declare(
    grpc
    GIT_REPOSITORY https://github.com/grpc/grpc
    GIT_TAG master
)
set(FETCHCONTENT_QUIT OFF)

set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)

FetchContent_MakeAvailable(grpc)

add_custom_command(
    OUTPUT helloworld.pb.cc
    COMMAND ${_PROTOBUF_PROTOC} -I${PROJECT_SOURCE_DIR} --cpp_out=${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/helloworld.proto
)

add_custom_command(
    OUTPUT helloworld.grpc.pb.cc
    COMMAND ${_PROTOBUF_PROTOC} -I${PROJECT_SOURCE_DIR} --grpc_out=${PROJECT_SOURCE_DIR} --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/helloworld.proto
)

add_executable(hello_world hello_world.cpp 
    ${PROJECT_SOURCE_DIR}/helloworld.grpc.pb.cc 
    ${PROJECT_SOURCE_DIR}/helloworld.pb.cc)

target_link_libraries(hello_world grpc++)
