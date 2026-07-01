#include "CallHandler.h"
#include <iostream>

CallHandler::CallHandler(user_service::UserService::AsyncService *service,
                         grpc::ServerCompletionQueue *cq,
                         PostgresConnectionPool *pool)
    : _service(service), _cq(cq), _pool(pool), _responder(&_ctx), _state(State::WAIT)
{
    // Register: tell gRPC we're ready to handle one Login RPC.
    // 'this' is the tag returned by cq->Next() when the request arrives.
    _service->RequestLogin(&_ctx, &_request, &_responder, _cq, _cq, this);
}

void CallHandler::proceed()
{
    if (_state == State::WAIT)
    {
        // Spawn successor before doing any work so the CQ can accept the next
        // request immediately while we process this one.
        new CallHandler(_service, _cq, _pool);

        _state = State::FINISH;
        grpc::Status status = handleLogin();
        _responder.Finish(_response, status, this); //putting task OMT in _cq so worker can decide wht to do with it (usually its FINISH and we just go to else branch)
    }
    else if (_state == State::FINISH)
    {
        delete this;
    }
}

grpc::Status CallHandler::handleLogin()
{
#ifdef NOOP_FLAG
    return grpc::Status::OK;
#endif

    if (!_pool)
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "DB not available");

    const auto &userData = _request.user();
    const std::string &email = userData.email();
    const std::string &inputPass = userData.password();

    try
    {
        PooledConnection conn(*_pool);
        pqxx::read_transaction txn(conn.get());
        pqxx::result r = txn.exec_params(
            "SELECT id, password FROM users WHERE email = $1", email);

        if (r.empty())
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found");

        int id = r[0][0].as<int>();
        std::string password = r[0][1].as<std::string>();

        if (inputPass != password)
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Wrong password");

        _response.set_user_id(id);
        _response.set_token("token_" + std::to_string(id));
        return grpc::Status::OK;
    }
    catch (const std::exception &e)
    {
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}
