if (EMSCRIPTEN OR MOBILE)
    message(FATAL_ERROR "WOLF_STREAM_GRPC feature is not supported for wasm or mobile targets.")
endif()

if (NOT WOLF_SYSTEM_OPENSSL)
    message(FATAL_ERROR "WOLF_STREAM_GRPC requires WOLF_SYSTEM_OPENSSL")
endif()

set(RPC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/wolf/stream/rpc)
file(GLOB_RECURSE RPC_SRCS)

list(APPEND RPC_SRCS
    ${RPC_PATH}/w_grpc_client.cpp
    ${RPC_PATH}/w_grpc_client.hpp
    # ${RPC_PATH}/w_grpc_server.cpp
    # ${RPC_PATH}/w_grpc_server.hpp
)

target_sources(${PROJECT_NAME} PRIVATE ${RPC_SRCS})

vcpkg_install(asio-grpc)
find_package(asio-grpc CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC asio-grpc::asio-grpc)

target_sources(${PROJECT_NAME}
    PRIVATE
    ${RPC_SRCS}
)
source_group("stream/grpc" FILES ${RPC_SRCS})

