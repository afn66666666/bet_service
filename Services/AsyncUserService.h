#pragma once

#include <grpcpp/grpcpp.h>
#include <vector>
#include <thread>
#include "user_service.grpc.pb.h"
#include "ConnectionPool.h"

/*!
 * \brief Async gRPC server for UserService.
 *
 * Owns the gRPC Server, AsyncService, N ServerCompletionQueues, and a thread
 * pool. Each thread runs an independent workerLoop() draining its own CQ.
 * Requests are dispatched via CallHandler state machines — no per-request
 * thread allocation.
 *
 * Usage:
 *   AsyncUserService svc("0.0.0.0:50051", dbCredentials, numThreads);
 *   svc.run();  // blocks until shutdown
 */
class AsyncUserService
{
public:
    AsyncUserService(const std::string &address,
                     const std::string &dbCredentials,
                     int numThreads);

    ~AsyncUserService();

    /*! \brief Start the server and block until shutdown. */
    void run();

private:
    void workerLoop(grpc::ServerCompletionQueue *cq);

    static constexpr int HANDLERS_PER_CQ = 200;

    int _numThreads;
    std::unique_ptr<PostgresConnectionPool> _pool;
    user_service::UserService::AsyncService _asyncService;
    std::vector<std::unique_ptr<grpc::ServerCompletionQueue>> _cqs;
    std::unique_ptr<grpc::Server> _server;
    std::vector<std::thread> _workers;
};
