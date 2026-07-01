#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <string>
#include <grpcpp/grpcpp.h>


#include "../Services/login_service/UserService.h"
#include "../Services/login_service/user_service.pb.h"
#include "../Services/login_service/user_service.grpc.pb.h"
#include "StressTester.h"
#include "DatabaseFiller.h"

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

using grpc::Channel;
using grpc::Status;

#define CONNECTION_TYPE_GRPC 1
constexpr int connectionType = 1;

int main()
{
   // auto stressTester = std::make_unique<StressTester>();
   // stressTester->testUserService();
   DatabaseFiller filler;
   filler.testFillUsers();
}