#pragma once

class IConnector{
    public:
    virtual ~IConnector() = default;
    virtual void initConnection() = 0;
    virtual void sendSignInRequest() = 0;
};