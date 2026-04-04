#pragma once
#include <iostream>
#include <ctime>
#include <thread>

#include "boost/asio.hpp"
#include "UserService.h"
#include "user_service.pb.h"
#include "user_service.grpc.pb.h"
#include "grpcpp/grpcpp.h"

namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;

#define RESPONSER_TYPE_GRPC 1
#define RESPONSER_TYPE_SOCKET 2

constexpr int responserType = 1;

int main()
{
    UserService us( "host=host.docker.internal port=5432 dbname=betting_db user=betting_admin password=kiba");

    std::string address = "0.0.0.0:50051";
    grpc::ServerBuilder serverBuilder;
    serverBuilder.AddListeningPort(address, grpc::InsecureServerCredentials());
    serverBuilder.RegisterService(&us);

    std::unique_ptr<grpc::Server> server(serverBuilder.BuildAndStart());
    std::cout << "Server listening on " << address << std::endl;
    server->Wait();
}