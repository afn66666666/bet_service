#pragma once

class IResponser
{
public:
    virtual ~IResponser() = default;
    virtual void handle() = 0;
};