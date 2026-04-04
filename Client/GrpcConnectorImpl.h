#pragma once
#include "IConnector.h"
#include "server_config.pb.h"
#include "server_config.grpc.pb.h"
#include "grpcpp/grpcpp.h"
#include <iostream>

class GrpcConnectorImpl : public IConnector
{
public:
    GrpcConnectorImpl() = default;
    void initConnection() override
    {
        _channel = grpc::CreateChannel("localhost:8080", grpc::InsecureChannelCredentials());
        _stub = server_config::Server::NewStub(_channel);
        if (!_stub)
        {
            std::cout << "Connection failed Type connection : grpc stub " << std::endl;
        }
        else
        {
            std::cout << "Connected to server. Type connection : grpc stub " << std::endl;
        }
    };
    virtual void sendSignInRequest() override
    {
        server_config::SignInRequest request;
        server_config::User *userRequest = request.mutable_user();
        userRequest->set_name("Ilya");
        userRequest->set_email("slicky003");
        userRequest->set_id(666);

        // response creation
        server_config::SignInResponse response;
        grpc::ClientContext context;

        // send rpc
        grpc::Status status = _stub->SignIn(&context, request, &response);
        if (status.ok())
        {
            std::cout << "sign in success : " << std::endl;
        }
        else
        {
            std::cout << "status.ok != true" << std::endl;
        }
    };

private:
    std::shared_ptr<grpc::Channel> _channel;
    std::unique_ptr<server_config::Server::Stub> _stub;
};