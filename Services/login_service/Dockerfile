FROM ubuntu:22.04

WORKDIR /app

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libprotobuf-dev \
    protobuf-compiler \
    protobuf-compiler-grpc \
    libgrpc++-dev \
    libpqxx-dev \
    libpq-dev \
    pkg-config \
    libboost-system-dev

COPY . .

# Regenerate pb files with the system protoc (3.12.4) to match the installed libprotobuf
RUN protoc -I . --cpp_out=. user_service.proto && \
    protoc -I . --grpc_out=. --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) user_service.proto

RUN mkdir build && cd build && \
    cmake .. && \
    make user_service -j

EXPOSE 50051

CMD ["./build/user_service"]