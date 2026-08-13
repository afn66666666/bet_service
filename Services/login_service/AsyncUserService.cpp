#include "AsyncUserService.h"
#include "CallHandler.h"
#include <iostream>
#include <stdexcept>
#include <utility>
#include <sodium.h>

AsyncUserService::AsyncUserService(const std::string& address,
                                   const std::string& dbCredentials,
                                   int numThreads,
                                   JwtConfig jwtConfig)
    : _numThreads(numThreads),
      _jwtSigner(std::make_unique<JwtTokenSigner>(std::move(jwtConfig)))
{
    // Init libsodium once, before workers start calling crypto_pwhash_str_verify.
    if (sodium_init() < 0)
        throw std::runtime_error("libsodium init failed");

    try
    {
        _pool = std::make_unique<PostgresConnectionPool>(dbCredentials, 50);
    }
    catch (...)
    {
        std::cerr << "DB connection pool failed — running without DB" << std::endl;
        _pool = nullptr;
    }

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&_asyncService);

    for (int i = 0; i < _numThreads; ++i)
        _cqs.push_back(builder.AddCompletionQueue());

    _server = builder.BuildAndStart();
    std::cout << "Async server listening on " << address
              << " (" << _numThreads << " threads)" << std::endl;
}

AsyncUserService::~AsyncUserService()
{
    _server->Shutdown();
    for (auto& cq : _cqs)
        cq->Shutdown();
    for (auto& t : _workers)
        if (t.joinable()) t.join();
}

void AsyncUserService::run()
{
    for (auto& cq : _cqs)
        for (int i = 0; i < HANDLERS_PER_CQ; ++i)
            new CallHandler(&_asyncService, cq.get(), _pool.get(), _jwtSigner.get());

    for (auto& cq : _cqs)
        _workers.emplace_back(&AsyncUserService::workerLoop, this, cq.get());

    for (auto& t : _workers)
        t.join();
}

void AsyncUserService::workerLoop(grpc::ServerCompletionQueue* cq)
{
    void* tag  = nullptr;
    bool  isOk = false;

    while (cq->Next(&tag, &isOk))
    {
        auto* handler = static_cast<CallHandler*>(tag);
        if (isOk)
            handler->proceed();
        else
            delete handler;  // CQ shutting down — clean up pending handlers
    }
}
