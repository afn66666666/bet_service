#pragma once
#include "IResponser.h"
#include "grpcpp/grpcpp.h"
#include "SignInServiceImpl.h"


class GrpcResponserImpl : public IResponser
{
public:
    GrpcResponserImpl() {
    };

    virtual void handle() override
    {
        std::string address = "0.0.0.0:8080";
        SignInServiceImpl dayService;
        grpc::ServerBuilder serverBuilder;
        serverBuilder.AddListeningPort(address, grpc::InsecureServerCredentials());
        serverBuilder.RegisterService(&dayService);

        std::unique_ptr<grpc::Server> server(serverBuilder.BuildAndStart());
        std::cout << "Server listening on " << address << std::endl;
        server->Wait();
    };
};