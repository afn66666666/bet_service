#include "AsyncUserService.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

static constexpr int NUM_THREADS = 16;

namespace
{
std::string requireEnvironmentVariable(const char *name)
{
    const char *value = std::getenv(name);
    if (!value || *value == '\0')
        throw std::runtime_error(std::string("Missing environment variable: ") + name);

    return value;
}

std::string readFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Failed to open file: " + path);

    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}
}

int main()
{
    JwtConfig jwtConfig{
        readFile(requireEnvironmentVariable("JWT_PUBLIC_KEY_PATH")),
        readFile(requireEnvironmentVariable("JWT_PRIVATE_KEY_PATH")),
        issuer : "login_service",
        audience : "bet_service",
        "login_key_test",
        std::chrono::seconds(900)};

    AsyncUserService service(
        "0.0.0.0:50051",
        "host=host.docker.internal port=5432 dbname=betting_db user=betting_admin password=kiba",
        NUM_THREADS,
        std::move(jwtConfig));

    service.run();
}
