#include "StressTester.h"
#include "MetaData.h"


#include <grpcpp/grpcpp.h>
#include <array>
#include <vector>
#include <random>
#include <iostream>

StressTester::StressTester()
{
    _channel = grpc::CreateChannel("localhost:6868",
                                       grpc::InsecureChannelCredentials());
    _stub = user_service::UserService::NewStub(_channel);     
}


void StressTester::worker()
{
    // auto channel = grpc::CreateChannel("localhost:6868",
    //                                    grpc::InsecureChannelCredentials());

    // auto stub = user_service::UserService::NewStub(channel);
    // while (true)
    // {
    //     user_service::LoginRequest request;
    //     auto data = generateLoginData(30);
    //     request.mutable_user()->set_email(data.first);
    //     request.mutable_user()->set_password(data.second);

    //     user_service::LoginResponse response;
    //     grpc::ClientContext context;

    //     auto status = stub->Login(&context, request, &response);

    //     if (status.ok())
    //     {
    //         _authorized++;
    //     }
    //     else if (status.error_code() == grpc::StatusCode::NOT_FOUND)
    //     {
    //         _non_authorized++;
    //     }
    //     else
    //     {
    //         auto msg = status.error_message();
    //         // std::cout << status.error_message() << std::endl;
    //         _failed++;
    //     }
    // }
}

void StressTester::refreshCounters()
{
    _authorized = 0;
    _non_authorized = 0;
    _failed = 0;
}

void StressTester::sendRequest()
{
        user_service::LoginRequest request;
        auto data = generateLoginData(30);
        request.mutable_user()->set_email(data.first);
        request.mutable_user()->set_password(data.second);

        AsyncCall* call = new AsyncCall;
        call->response_reader = _stub->PrepareAsyncLogin(&call->context,request,&_complQueue);

        call->response_reader->StartCall();
        call->response_reader->Finish(&call->reply,&call->status,(void*)call);

}

void StressTester::run(int requestsPerThread)
{
      for (int i = 0; i < requestsPerThread; ++i) {
            sendRequest();
        }

        void* tag;
        bool isOk;
        while(_complQueue.Next(&tag,&isOk)){
            auto call = static_cast<AsyncCall*>(tag);

            if(isOk && call->status.ok()){
                _authorized++;
            }
            else if(call->status.error_code() == grpc::StatusCode::NOT_FOUND){
                _non_authorized++;

            }
            else{
                _failed++;
            }

            delete call;

            sendRequest();
        }
}

std::pair<std::string, std::string> StressTester::generateLoginData(int chance) const
{
    std::string email = "invalid_gmail";
    std::string pass = "invalid_password";

    thread_local std::mt19937 gen(std::random_device{}());
    // std::uniform_int_distribution<int> randVal(1, 100);
    // std::uniform_int_distribution<int> indexDist(0, emails.size() - 1);
    // int chanceVal = randVal(gen);
    // if (chanceVal >= chance)
    if(true)
    {
        // int ii = indexDist(gen);
        email = emails[70];
        pass = passwords[70];
    }
    return {email, pass};
}


void StressTester::testUserService()
{
    // int threads = 50;

    // std::vector<std::thread> pool;

    // for (int i = 0; i < threads; ++i)
    // {
    //     pool.emplace_back(&StressTester::worker, this);
    // }

    // while (true)
    // {
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    //     std::cout << "RPS ~ "
    //               << _authorized.load() << " authorized, "
    //               << _non_authorized.load() << " invalid login\\pass, "
    //               << _failed.load() << " failed"
    //               << std::endl;

    //     refreshCounters();
    // }

    // for (auto &t : pool)
    //     t.join();

     std::thread t([this]() {
        run(6000); // стартовое количество in-flight
    });

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "RPS ~ "
                  << _authorized.load() << " authorized, "
                  << _non_authorized.load() << " invalid login\\pass, "
                  << _failed.load() << " failed"
                  << std::endl;

        refreshCounters();
    }

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
