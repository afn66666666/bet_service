#pragma once
#include "IConnector.h"
#include <boost/asio.hpp>
#include <iostream>

#include "server_config.pb.h"

boost::asio::io_context io;
boost::asio::ip::tcp::socket _socket(io);

    class RawSocketConnectorImpl : public IConnector
{
public:
    RawSocketConnectorImpl() = default;
    virtual void initConnection() override
    {
        boost::asio::io_context io;
        boost::asio::ip::tcp::resolver resolver(io);
        auto endpoints = resolver.resolve("127.0.0.1", "8080");
        std::cout << "Connected to server. Type connection : raw socket" << std::endl;
    };
    virtual void sendSignInRequest() override
    {
        server_config::User userMessage;
        userMessage.set_name("Ilya");
        userMessage.set_email("slicky003");
        userMessage.set_id(666);
        std::string payload;
        userMessage.SerializeToString(&payload);
        _socket.write_some(boost::asio::buffer(payload));

        
    };
};