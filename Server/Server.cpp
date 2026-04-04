#pragma once
#include <iostream>
#include <ctime>
#include <thread>

#include "boost/asio.hpp"
#include "server_config.pb.h"
#include "IResponser.h"
#include "GrpsResponserImpl.h"
#include "TcpResponserImpl.h"
namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;

#define RESPONSER_TYPE_GRPC 1
#define RESPONSER_TYPE_SOCKET 2

constexpr int responserType = 1;


int main()
{
    std::cout << "start" << std::endl;
    std::unique_ptr<IResponser> server;
    if(responserType == RESPONSER_TYPE_GRPC){
        server = std::make_unique<GrpcResponserImpl>();
    }
    else if(responserType == RESPONSER_TYPE_SOCKET){
        server = std::make_unique<TcpResponserImpl>();
    }

    server->handle();
}