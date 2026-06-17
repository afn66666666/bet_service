#include "UserService.h"

UserService::UserService(const std::string &credentials)
{

    try
    {
        _pool = std::make_unique<PostgresConnectionPool>(credentials, 50);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        _pool = nullptr;
    }
}