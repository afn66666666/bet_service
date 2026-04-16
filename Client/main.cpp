#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <string>
#include <grpcpp/grpcpp.h>


#include "../Services/UserService.h"
#include "../Services/user_service.pb.h"
#include "../Services/user_service.grpc.pb.h"
#include "StressTester.h"

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

using grpc::Channel;
using grpc::Status;

#define CONNECTION_TYPE_GRPC 1
#define CONNECTION_TYPE_RAW_TCP 2
constexpr int connectionType = 1;

int main()
{
   auto stressTester = std::make_unique<StressTester>();
   stressTester->testUserService();

}