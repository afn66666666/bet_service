

#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

#include "../Services/login_service/UserService.h"
#include "../Services/login_service/user_service.pb.h"
#include "../Services/login_service/user_service.grpc.pb.h"

static constexpr int NUM_THREADS = 16;
const int requestsPerThread = 1;

/*!
 * \brief Async gRPC load generator for login_service (Login RPC).
 *
 * Spawns NUM_THREADS worker threads, each with its own gRPC channel, stub,
 * and CompletionQueue. Each thread keeps requestsPerThread async Login RPCs
 * in flight, processing completions in a tight loop — a fixed window of
 * concurrent requests.
 *
 * Reports RPS, authorized/failed counts, and average latency once per second.
 *
 * generateLoginData() picks a random valid user (emails[i]/passwords[i]).
 */
class LoginSimulator
{
public:
    LoginSimulator();
    void testUserService();
    std::pair<std::string, std::string> generateLoginData(int chance) const;

private:
    struct AsyncCall
    {
        user_service::LoginResponse reply;
        grpc::ClientContext context;
        grpc::Status status;
        std::unique_ptr<grpc::ClientAsyncResponseReader<user_service::LoginResponse>> response_reader;
        std::chrono::steady_clock::time_point start_time;
    };
    void refreshCounters();
    void sendRequest(int threadId);
    void run(int requestsPerThread, int threadId);

    grpc::CompletionQueue _queues[NUM_THREADS];
    std::thread _workers[NUM_THREADS];
    std::shared_ptr<grpc::Channel> _channels[NUM_THREADS];
    std::unique_ptr<user_service::UserService::Stub> _stubs[NUM_THREADS];

    std::atomic<int> _authorized{0};
    std::atomic<int> _non_authorized{0};
    std::atomic<int> _failed{0};

    std::atomic<int64_t> _total_latency_ns{0};
    std::atomic<int64_t> _completed{0};
};
