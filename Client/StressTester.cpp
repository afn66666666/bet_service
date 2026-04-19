#include "StressTester.h"
#include "MetaData.h"
#include "../Services/UserService.h"
#include "../Services/user_service.pb.h"
#include "../Services/user_service.grpc.pb.h"

#include <grpcpp/grpcpp.h>
#include <array>
#include <vector>
#include <random>
#include <iostream>

void StressTester::worker()
{
    auto channel = grpc::CreateChannel("localhost:6868",
                                       grpc::InsecureChannelCredentials());

    auto stub = user_service::UserService::NewStub(channel);
    while (true)
    {
        user_service::LoginRequest request;
        auto data = generateLoginData(30);
        request.mutable_user()->set_email(data.first);
        request.mutable_user()->set_password(data.second);

        user_service::LoginResponse response;
        grpc::ClientContext context;

        auto status = stub->Login(&context, request, &response);

        if (status.ok())
        {
            authorized++;
        }
        else if (status.error_code() == grpc::StatusCode::NOT_FOUND)
        {
            non_authorized++;
        }
        else
        {
            auto msg = status.error_message();
            // std::cout << status.error_message() << std::endl;
            failed++;
        }
    }
}

void StressTester::refreshCounters()
{
    authorized = 0;
    non_authorized = 0;
    failed = 0;
}

std::pair<std::string, std::string> StressTester::generateLoginData(int chance) const
{
    std::string email = "invalid_gmail";
    std::string pass = "invalid_password";

    thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> randVal(1, 100);
    std::uniform_int_distribution<int> indexDist(0, emails.size() - 1);
    int chanceVal = randVal(gen);
    if (chanceVal >= chance)
    {
        int ii = indexDist(gen);
        email = emails[ii];
        pass = passwords[ii];
    }
    return {email, pass};
}

void StressTester::testUserService()
{

    int threads = 50;

    std::vector<std::thread> pool;

    for (int i = 0; i < threads; ++i)
    {
        pool.emplace_back(&StressTester::worker, this);
    }

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "RPS ~ "
                  << authorized.load() << " authorized, "
                  << non_authorized.load() << " invalid login\\pass, "
                  << failed.load() << " failed"
                  << std::endl;

        refreshCounters();
    }

    for (auto &t : pool)
        t.join();
}

void utilDatabaseConnection()
{
    // try
    // {
    //     std::string dbCredentials = "host = " + host + " "
    //                                                    "port= " +
    //                                 port + " "
    //                                        "dbname= " +
    //                                 dbName + " "
    //                                          "user=betting_admin "
    //                                          "password=kiba";

    //     connector = std::make_unique<pqxx::connection>(dbCredentials.data());
    // }
    // catch (const std::exception &e)
    // {
    //     std::cerr << "Error: " << e.what() << std::endl;
    // }
}
