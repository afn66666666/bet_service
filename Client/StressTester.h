

#include <iostream>
#include <memory>


#include "../Services/UserService.h"
#include "../Services/user_service.pb.h"
#include "../Services/user_service.grpc.pb.h"

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
    };
    void worker();
    void refreshCounters();
    void sendRequest();
    void run(int requestsPerThread);
    std::pair<std::string, std::string> getRandomData() const;
    std::shared_ptr<grpc::Channel> _channel;
    std::unique_ptr<user_service::UserService::Stub> _stub{};
    grpc::CompletionQueue _complQueue;
    std::atomic<int> _authorized{0};
    std::atomic<int> _non_authorized{0};
    std::atomic<int> _failed{0};
};
