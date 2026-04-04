FROM ubuntu:22.04
WORKDIR /app

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    protobuf-compiler \
    libprotobuf-dev \
    grpc++ \
    libgrpc++-dev

# Сделать ссылку для плагина, чтобы protoc нашел grpc-плагин под нужным именем
RUN ln -s /usr/bin/grpc_cpp_plugin /usr/bin/protoc-gen-grpc


COPY Server ./Server
COPY user.proto .

RUN protoc \
    --grpc_out=. \
    --cpp_out=. \
    user.proto

RUN g++ -std=gnu++17 \
    /app/Server/wipi.cpp \
    user.pb.cc \
    user.grpc.pb.cc \
    -o wipi \
    -lgrpc++ \
    -lprotobuf \
    -lboost_system \
    -lpthread

EXPOSE 8080
CMD ["./wipi"]
