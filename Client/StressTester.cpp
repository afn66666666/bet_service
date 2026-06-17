#include "StressTester.h"
#include "MetaData.h"

#include <grpcpp/grpcpp.h>
#include <array>
#include <vector>
#include <random>
#include <iostream>

StressTester::StressTester()
{
    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_MAX_CONCURRENT_STREAMS, 0);

    for (int i = 0; i < NUM_THREADS; i++)
    {
        _channels[i] = grpc::CreateCustomChannel("localhost:6868",
                                                 grpc::InsecureChannelCredentials(),
                                                 args);
        _stubs[i] = user_service::UserService::NewStub(_channels[i]);
    }
}

void StressTester::refreshCounters()
{
    _authorized = 0;
    _non_authorized = 0;
    _failed = 0;

    _total_latency_ns = 0;
    _completed = 0;
}

void StressTester::sendRequest(int threadId)
{
    user_service::LoginRequest request;
    auto data = generateLoginData(30);
    request.mutable_user()->set_email(data.first);
    request.mutable_user()->set_password(data.second);

    AsyncCall *call = new AsyncCall;
    call->start_time = std::chrono::steady_clock::now();
    call->response_reader = _stubs[threadId]->PrepareAsyncLogin(
        &call->context, request, &_queues[threadId]);

    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void *)call);
}

void StressTester::run(int requestsPerThread, int threadId)
{
    for (int i = 0; i < requestsPerThread; ++i)
    {
        sendRequest(threadId);
    }

    void *tag;
    bool isOk;
    while (_queues[threadId].Next(&tag, &isOk))
    {
        auto call = static_cast<AsyncCall *>(tag);

        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - call->start_time);
        _total_latency_ns.fetch_add(latency.count());
        _completed.fetch_add(1);

        if (isOk && call->status.ok())
        {
            _authorized++;
        }
        else if (call->status.error_code() == grpc::StatusCode::NOT_FOUND)
        {
            _non_authorized++;
        }
        else
        {
            _failed++;
        }

        delete call;
        sendRequest(threadId);
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
    if (true)
    {
        // int ii = indexDist(gen);
        email = emails[70];
        pass = passwords[70];
    }
    return {email, pass};
}

void StressTester::testUserService()
{
    const int requestsPerThread = 2000;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        _workers[i] = std::thread(&StressTester::run, this, requestsPerThread, i);
    }

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        int64_t comp = _completed.load();
        double avg_ms = comp > 0 ? (_total_latency_ns.load() / comp) / 1e6 : 0;
        std::cout << "RPS ~ "
                  << _authorized.load() << " authorized, "
                  << _non_authorized.load() << " invalid login\\pass, "
                  << _failed.load() << " failed, "
                  << avg_ms << " ms avg latency"
                  << std::endl;
        refreshCounters();
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        _workers[i].join();
    }
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
