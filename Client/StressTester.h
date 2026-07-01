

#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

#include "../Services/login_service/UserService.h"
#include "../Services/login_service/user_service.pb.h"
#include "../Services/login_service/user_service.grpc.pb.h"

static constexpr int NUM_THREADS = 16;
const int requestsPerThread = 128;

/*!
 * \brief Async gRPC load tester for UserService.
 *
 * Spawns NUM_THREADS worker threads, each with its own gRPC channel, stub,
 * and CompletionQueue. Each thread fires requestsPerThread async Login RPCs
 * upfront, then processes completions in a tight loop — maintaining a fixed
 * window of in-flight requests for maximum throughput.
 *
 * Reports RPS, authorized/failed counts, and average latency once per second.
 *
 * \note generateLoginData() currently hardcodes emails[70] (randomization
 *       is commented out) — all requests hit a single user record.
 */
class StressTester
{
public:
    StressTester();
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
