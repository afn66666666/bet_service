#pragma once

#include <atomic>
#include "pqxx/pqxx"
#include "user_service.grpc.pb.h"
#include "user_service.pb.h"
#include "ConnectionPool.h"

#define NOOP_FLAG

/*!
 * \brief gRPC service implementation for UserService.Login RPC (user_service.proto).
 *
 * Handles authentication requests: validates email/password against PostgreSQL,
 * returns a session token on success. Uses PostgresConnectionPool for efficient
 * concurrent DB access.
 *
 * When NOOP_FLAG is defined, Login returns OK immediately without touching the
 * database — used to measure the upper bound of pure gRPC throughput.
 */
class UserService : public user_service::UserService::Service
{

public:
    UserService(const std::string &credentials);

    /*! \brief Authenticates a user by email and password. Returns a token on success. */
    grpc::Status Login(grpc::ServerContext *context, const user_service::LoginRequest *request, user_service::LoginResponse *response) override
    {
        static std::atomic<bool> firstConnection{true};
        if (firstConnection.exchange(false))
            std::cout << "First client connected: " << context->peer() << std::endl;

        #ifdef NOOP_FLAG
            return grpc::Status::OK;
        #endif

        if (!_pool)
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "DB not available");
        }

        auto userData = request->user();
        std::string email = userData.email();
        std::string inputPass = userData.password();
        auto connection = PooledConnection(*_pool);
        pqxx::read_transaction txn(connection.get());
        pqxx::result r = txn.exec_params(
            "SELECT id, password FROM users WHERE email = $1", email);

        if (r.empty())
        {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found");
        }

        std::string password = r[0][1].as<std::string>();
        int id = r[0][0].as<int>();
        if (inputPass == password)
        {
            // token creation
            std::string token = "token_" + std::to_string(id);
            response->set_user_id(id);
            response->set_token(token);

            return grpc::Status::OK;
        }

        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Wrong password");
    }

private:
    std::unique_ptr<PostgresConnectionPool> _pool;
};