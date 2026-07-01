#include <grpc++/grpc++.h>
#include <sstream>
#include <iomanip>
#include <ctime>

#include "server_config.grpc.pb.h"

/*!
 * \brief gRPC service implementation for the Server.SignIn RPC (server_config.proto).
 *
 * Handles user sign-in requests from the gateway Server component (port 8080).
 * Currently logs user info and echoes back the user id with success=true.
 * Intended to be extended with actual authentication logic.
 */
class SignInServiceImpl : public server_config::Server::Service
{
public:
    grpc::Status SignIn(grpc::ServerContext *context, const server_config::SignInRequest *request, server_config::SignInResponse *response) override
    {
        auto user = request->user();

        std::cout << "User connected:\n";
        std::cout << "id: " << user.id() << "\n";
        std::cout << "name: " << user.name() << "\n";
        std::cout << "email: " << user.email() << "\n";

        response->set_success(true);
        response->set_id(user.id());
        return grpc::Status::OK;
    }
};