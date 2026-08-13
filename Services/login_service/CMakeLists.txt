cmake_minimum_required(VERSION 3.20)
project(UserService CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "Build jwt-cpp examples" FORCE)
FetchContent_Declare(
    jwt-cpp
    URL https://github.com/Thalhammer/jwt-cpp/archive/refs/tags/v0.7.2.tar.gz
    URL_HASH SHA512=9a2725228565d671e065a4647dad38f36251a4ee07c796cac35252557134a20c2dc260f62c011438c7fbde57f5c511bb0096569512c0aebdae048c7a626805b7
)
FetchContent_MakeAvailable(jwt-cpp)

# ── Зависимости ─────────────────────────────────────────────

find_package(Protobuf REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/user_service.proto)
set(PROTO_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/user_service.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/user_service.grpc.pb.cc
)
set(PROTO_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/user_service.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/user_service.grpc.pb.h
)

find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin REQUIRED)

add_custom_command(
    OUTPUT ${PROTO_SOURCES} ${PROTO_HEADERS}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS
        --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
        --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
        --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
        --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
        ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    VERBATIM
)

# ── target ──────────────────────────────────────────────────

add_executable(user_service
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AsyncUserService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CallHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/JWTTokenSigner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/JWTTokenSigner.h
    ${PROTO_SOURCES}
    ${PROTO_HEADERS}
)

target_include_directories(user_service PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(user_service PRIVATE
    grpc++
    grpc
    protobuf
    pqxx
    pq
    sodium
    Boost::system
    jwt-cpp::jwt-cpp
)
