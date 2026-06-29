cmake_minimum_required(VERSION 3.20)
project(UserService CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Зависимости ─────────────────────────────────────────────

find_package(Protobuf REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# ── target ──────────────────────────────────────────────────

add_executable(user_service
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AsyncUserService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CallHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/user_service.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/user_service.grpc.pb.cc
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
    Boost::system
)