#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <string>
#include <grpcpp/grpcpp.h>

#include "IConnector.h"
#include "GrpcConnectorImpl.h"
#include "RawSocketConnectorImpl.h";
#include "PostgresConnector.h"
#include "../Services/UserService.h"
#include "../Services/user_service.pb.h"
#include "../Services/user_service.grpc.pb.h"

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

using grpc::Channel;
using grpc::Status;

#define CONNECTION_TYPE_GRPC 1
#define CONNECTION_TYPE_RAW_TCP 2
constexpr int connectionType = 1;

int main()
{
    // std::unique_ptr<IConnector> connector;

    // if (connectionType == CONNECTION_TYPE_GRPC)
    // {
    //     connector = std::make_unique<GrpcConnectorImpl>();
    // }
    // else if (connectionType == CONNECTION_TYPE_RAW_TCP)
    // {
    //     connector = std::make_unique<RawSocketConnectorImpl>();
    // }

    // connector->initConnection();
    // connector->sendSignInRequest();

    auto channel = grpc::CreateChannel("localhost:6868", grpc::InsecureChannelCredentials());
    auto stub = user_service::UserService::NewStub(channel);
    user_service::LoginRequest req;
    user_service::LoginResponse response;
    auto user = req.mutable_user();
    user->set_email("KNIGHT_BOLTON_231sdsds6@gmail.com");
    user->set_password("pass_2316");
    grpc::ClientContext context;
    auto status = stub->Login(&context, req, &response);
    if (status.ok())
    {
        std::cout << response.token() << std::endl;
    }
    else
        std::cout << status.error_message() << std::endl;
    // try
    // {
    //     asio::io_context io;

    //     //grpc endpoint creation
    //     auto channel = grpc::CreateChannel("localhost:8080", grpc::InsecureChannelCredentials());
    //     auto stub = day::DayService::NewStub(channel);

    //     //request creation
    //     day::GetCurrentDayRequest request;
    //     day::User* userRequest = request.mutable_user();
    //     userRequest->set_name("Ilya");
    //     userRequest->set_email("slicky003");
    //     userRequest->set_id(666);

    //     //response creation
    //     day::GetCurrentDayResponse response;
    //     grpc::ClientContext context;

    //     //send rpc
    //     grpc::Status status = stub->GetCurrentDay(&context, request,&response);

    //     if(status.ok()){
    //         const day::Day& day = response.day();
    //     std::cout << "Day info:\n";
    //     std::cout << "day: " << day.day() << "\n";
    //     std::cout << "month: " << day.month() << "\n";
    //     std::cout << "weekday: " << day.day_of_week() << "\n";
    //     std::cout << "time: " << day.time() << "\n";
    //     }

    //     // tcp socket creation
    //     tcp::resolver resolver(io);
    //     auto endpoints = resolver.resolve("127.0.0.1", "8080");

    //     tcp::socket socket(io);
    //     asio::connect(socket, endpoints);

    //     // handshake
    //     std::cout << "Connected to server" << std::endl;

    //     // sending message
    //     //  boost::system::error_code error;
    //     //  std::cout << "Say something to server! : ";
    //     //  std::string messageToServer;
    //     //  std::cin >> messageToServer;
    //     //  socket.write_some(asio::buffer(messageToServer),error);

    //     // sending User protobuf
    //     day::User userMessage;
    //     userMessage.set_name("Ilya");
    //     userMessage.set_email("slicky003");
    //     userMessage.set_id(666);
    //     std::string payload;
    //     userMessage.SerializeToString(&payload);
    //     socket.write_some(asio::buffer(payload));

    //     // getting answer
    //     boost::system::error_code error;
    //     char serverAnswer[512];
    //     size_t len = socket.read_some(asio::buffer(serverAnswer), error);
    //     if (error)
    //         std::cout << "socket.read() error";
    //     // day::Day response;
    //     // std::cout << "Server answer (raw payload): " << std::string(serverAnswer,len) << std::endl;
    //     // if(!response.ParseFromArray(serverAnswer,len)){
    //     //     std::cout << "ParseFromArray() failed" << std::endl;
    //     // }
    //     // std::cout << "formatted data : " << response.time() << " " << response.day_of_week() << " " << response.day() << " " << response.month();
    //     // std::cout << std::endl;
    // }
    // catch (const boost::system::system_error &e)
    // {
    //     const auto &ec = e.code();

    //     std::cerr
    //         << "error value: " << ec.value() << "\n"
    //         << "category   : " << ec.category().name() << "\n";
    // }
}