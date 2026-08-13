#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "jwt-cpp/jwt.h"

struct JwtConfig
{
    std::string publicKey;
    std::string privateKey;
    std::string issuer;
    std::string audience;
    std::string keyId;
    std::chrono::seconds ttl;
};

class JwtTokenSigner
{
public:
    explicit JwtTokenSigner(JwtConfig config);

    std::string issueAccessToken(int64_t userId) const;

private:
    JwtConfig _config;
};
