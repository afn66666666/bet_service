#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <string>
#include <grpcpp/grpcpp.h>


#include "../Services/login_service/UserService.h"
#include "../Services/login_service/user_service.pb.h"
#include "../Services/login_service/user_service.grpc.pb.h"
#include "LoginSimulator.h"
#include "DatabaseFiller.h"

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

using grpc::Channel;
using grpc::Status;

#define CONNECTION_TYPE_GRPC 1
constexpr int connectionType = 1;

int main()
{
   auto loginSim = std::make_unique<LoginSimulator>();
   loginSim->testUserService();
   
   // DatabaseFiller filler;
   // filler.testFillUsers();
}