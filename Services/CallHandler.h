#include <pqxx/pqxx>
#include <memory>
#include <grpcpp/grpcpp.h>

#include "UserService.h"
#include "user_service.pb.h"
#include "user_service.grpc.pb.h"
#include "ConnectionPool.h"

/*!
 * \brief Per-request state machine for async gRPC Login RPC handling.
 *
 * Each instance represents one in-flight RPC call. On construction it
 * registers itself with the CompletionQueue to receive the next incoming
 * request. When the CQ fires, proceed() advances the state:
 *   WAIT -> PROCESS: spawn a successor handler, execute login logic, send reply
 *   FINISH: delete this
 *
 * Instances are heap-allocated and self-deleting. The caller must not
 * hold a pointer after invoking proceed() in the FINISH state.
 */class CallHandler
{
public:
    CallHandler(user_service::UserService::AsyncService *service,
                grpc::ServerCompletionQueue *cq,
                PostgresConnectionPool *pool);
    enum class State
    {
        WAIT,
        PROCESS,
        FINISH
    };

    // вызывается из workerLoop при каждом событии из cq
    void proceed();

private:
grpc::Status handleLogin();


private:
    user_service::UserService::AsyncService *_service;
    grpc::ServerCompletionQueue *_cq;
    grpc::ServerContext _ctx;
    user_service::LoginRequest _request;
    user_service::LoginResponse _response;
    grpc::ServerAsyncResponseWriter<user_service::LoginResponse> _responder;
    State _state = State::WAIT;
    PostgresConnectionPool *_pool; // не владеет, borrowed ptr
};