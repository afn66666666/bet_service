FROM ubuntu:22.04

WORKDIR /app

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libprotobuf-dev \
    protobuf-compiler \
    protobuf-compiler-grpc \
    libgrpc++-dev \
    libpq-dev \
    libpqxx-dev \
    libboost-system-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*
# RUN apt-get update && apt-get install -y \
#     libprotobuf-dev \
#     protobuf-compiler \
#     && rm -rf /var/lib/apt/lists/*

COPY . .

RUN mkdir build && cd build && \
    cmake .. && \
    make user_service -j

EXPOSE 50051

CMD ["./build/user_service"]