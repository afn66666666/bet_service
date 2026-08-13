#include "LoginSimulator.h"
#include "MetaData.h"

#include <grpcpp/grpcpp.h>
#include <array>
#include <vector>
#include <random>
#include <iostream>

LoginSimulator::LoginSimulator()
{
    // Unique arg per channel so gRPC does not share one subchannel across
    // all channels — otherwise all 16 collapse into a single TCP connection
    for (int i = 0; i < NUM_THREADS; i++)
    {
        grpc::ChannelArguments args;
        args.SetInt("unique_conn_id", i);

        _channels[i] = grpc::CreateCustomChannel("localhost:6868",
                                                 grpc::InsecureChannelCredentials(),
                                                 args);
        _stubs[i] = user_service::UserService::NewStub(_channels[i]);
    }
}

void LoginSimulator::refreshCounters()
{
    _authorized = 0;
    _non_authorized = 0;
    _failed = 0;

    _total_latency_ns = 0;
    _completed = 0;
}

void LoginSimulator::sendRequest(int threadId)
{
    user_service::LoginRequest request;
    auto data = generateLoginData(0);
    request.mutable_user()->set_email(data.first);
    request.mutable_user()->set_password(data.second);

    AsyncCall *call = new AsyncCall;
    call->start_time = std::chrono::steady_clock::now();
    call->response_reader = _stubs[threadId]->PrepareAsyncLogin(
        &call->context, request, &_queues[threadId]);

    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void *)call);
}

void LoginSimulator::run(int requestsPerThread, int threadId)
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

std::pair<std::string, std::string> LoginSimulator::generateLoginData(int chance) const
{
    thread_local std::mt19937 gen(std::random_device{}());
    thread_local std::uniform_int_distribution<int> randVal(1, 100);
    thread_local std::uniform_int_distribution<size_t> indexDist(0, emails.size() - 1);

    // 'chance'% of requests use bogus credentials (exercise the NOT_FOUND path);
    // the rest pick a random valid user. emails[i] and passwords[i] are index-
    // aligned with what DatabaseFiller hashed into the DB, so the login succeeds.
    if (randVal(gen) <= chance)
        return {"invalid_gmail", "invalid_password"};

    size_t i = indexDist(gen);
    return {emails[i], passwords[i]};
}

void LoginSimulator::testUserService()
{

    for (int i = 0; i < NUM_THREADS; i++)
    {
        _workers[i] = std::thread(&LoginSimulator::run, this, requestsPerThread, i);
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
