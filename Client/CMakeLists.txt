cmake_minimum_required(VERSION 3.20)

# Если не передан снаружи — подключаем vcpkg автоматически
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
     set(CMAKE_TOOLCHAIN_FILE "L:/programming/cpp/vcpkg/scripts/buildsystems/vcpkg.cmake"
         CACHE STRING "vcpkg toolchain")
endif()

project(client CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(gRPC REQUIRED)
# генерация protobuf

# ── Зависимости ──────────────────────────────────────────────────────────────
 find_package(gRPC        CONFIG REQUIRED)
 find_package(Protobuf    CONFIG REQUIRED)
find_package(Boost       REQUIRED COMPONENTS system)
 find_package(libpqxx     CONFIG REQUIRED)

# ── Сгенерированные protobuf-файлы (общие для клиента и сервера) ──────────────
set(PROTO_SOURCES
        ../Server/server_config.pb.cc
        ../Server/server_config.grpc.pb.cc
        ../Services/user_service.pb.cc
        ../Services/user_service.grpc.pb.cc
)

# ── client ────────────────────────────────────────────────────────────────
add_executable(client
    main.cpp
    StressTester.cpp
     ${PROTO_SOURCES}
)

target_include_directories(client PRIVATE
    Client
    ../Server   # server_config.pb.h / server_config.grpc.pb.h
    ../Services
)

target_link_libraries(client PRIVATE
    gRPC::grpc++
    # grpc++
    # grpc
    protobuf::libprotobuf
    # protobuf
    Boost::system
    libpqxx::pqxx
    # pqxx
    # pq
)
