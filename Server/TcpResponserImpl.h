#pragma once
#include "IResponser.h"
#include <boost/asio.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

#include "server_config.pb.h"

namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;

class TcpResponserImpl : public IResponser
{
public:
    TcpResponserImpl() {};
    virtual void handle() override
    {
        boost::asio::io_context io;
        boost::asio::ip::tcp::acceptor acceptor(io, boost::asio::ip::tcp::endpoint(tcp::v4(), 8080));
        try
        {
            for (;;)
            {
                tcp::socket socket(io);
                acceptor.accept(socket);

                auto localIp = socket.local_endpoint().address().to_string();
                auto localPort = socket.local_endpoint().port();

                auto clientIp = socket.remote_endpoint().address().to_string();
                auto clientPort = socket.remote_endpoint().port();

                std::string connectionMsg = "INIT: new connection : " + clientIp + ":" + std::to_string(clientPort) + " -> " + localIp + ":" + std::to_string(localPort);
                std::cout << connectionMsg << std::endl;

                // read user from client
                char data[512];
                boost::system::error_code error;
                size_t len = socket.read_some(asio::buffer(data), error);
                server_config::User userPayload;
                if (!userPayload.ParseFromArray(data, len))
                {
                    std::cout << "error : parsing user.proto failed" << std::endl;
                }

                std::cout << "info about user " << std::endl;
                std::cout << "id : " << userPayload.id() << std::endl;
                std::cout << "name : " << userPayload.name() << std::endl;
                std::cout << "email : " << userPayload.email() << std::endl;

                std::cout << "raw payload : " << "\"" << std::string(data, len) << "\"" << std::endl;

                // creating answer
                // day::Day dayResponse;
                // dayResponse.set_day_of_week("Tuesday");
                // dayResponse.set_day(3);
                // dayResponse.set_month("Feb");
                // auto now = std::chrono::system_clock::now();

                // std::time_t now_time = std::chrono::system_clock::to_time_t(now);

                // std::tm *local_tm = std::localtime(&now_time);

                // std::ostringstream oss;
                // oss << std::put_time(local_tm, "%H:%M");
                // dayResponse.set_time(oss.str());

                // std::string rawPayload = dayResponse.SerializeAsString();
                // boost::system::error_code ignored_error;
                // boost::asio::write(socket, boost::asio::buffer(rawPayload), ignored_error);
            }
        }
        catch (const boost::system::system_error &e)
        {
            std::cerr << "SERVER ERROR: " << e.what() << std::endl;
        }
    };
};