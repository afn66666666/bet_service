#pragma once

/*!
 * \brief Abstract transport layer interface.
 *
 * Strategy pattern: allows switching between gRPC and raw TCP transport
 * without changing the entry point in main(). Concrete implementations
 * own the server lifecycle and block inside handle() until shutdown.
 */
class IResponser
{
public:
    virtual ~IResponser() = default;

    /*! \brief Start the server and block until it shuts down. */
    virtual void handle() = 0;
};