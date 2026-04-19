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

RUN mkdir build && cd build && \
    cmake .. && \
    make user_service -j

EXPOSE 50051

CMD ["./build/user_service"]