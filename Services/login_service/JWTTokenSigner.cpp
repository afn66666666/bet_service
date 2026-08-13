#include "JWTTokenSigner.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>


JwtTokenSigner::JwtTokenSigner(JwtConfig config)
: _config(std::move(config))
{
}

std::string JwtTokenSigner::issueAccessToken(int64_t userId) const
{
    auto now = std::chrono::system_clock::now();
    return jwt::create()
        .set_type("at+jwt")
        .set_key_id(_config.keyId)
        .set_issuer(_config.issuer)
        .set_audience(_config.audience)
        .set_subject(std::to_string(userId))
        .set_issued_at(now)
        .set_expires_at(now + _config.ttl)
        .sign(jwt::algorithm::rs256{_config.publicKey, _config.privateKey});
}