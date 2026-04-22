#pragma once

#include "pqxx/pqxx"
#include "user_service.grpc.pb.h"
#include "user_service.pb.h"
#include "ConnectionPool.h"

#define NOOP_FLAG

class UserService : public user_service::UserService::Service
{

public:
    UserService(const std::string &credentials);
   

    grpc::Status Login(grpc::ServerContext *context, const user_service::LoginRequest *request, user_service::LoginResponse *response) override
    {

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
    std::unique_ptr<ConnectionPool> _pool;
};