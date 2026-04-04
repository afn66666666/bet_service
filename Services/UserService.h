#include "pqxx/pqxx"
#include "user_service.grpc.pb.h"
#include "user_service.pb.h"

class UserService : public user_service::UserService::Service
{

public:

    public:
    UserService(const std::string& credentials) {

         try
        {

            connector = std::make_unique<pqxx::connection>(credentials.data());
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    grpc::Status Login(grpc::ServerContext *context, const user_service::LoginRequest *request, user_service::LoginResponse *response) override
    {
        if (!connector || !connector->is_open())
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "DB not available");
        }
        
        auto userData = request->user();
        std::string email = userData.email();
        std::string inputPass = userData.password();
        std::cout << "got login request from : " << email << std::endl;
        pqxx::read_transaction txn(*connector);
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
    std::unique_ptr<pqxx::connection> connector;
};