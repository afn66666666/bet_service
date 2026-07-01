#pragma once
#include <pqxx/pqxx>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <iostream>

/*!
 * \brief Thread-safe PostgreSQL connection pool.
 *
 * Pre-allocates a fixed number of pqxx::connection objects at construction.
 * Threads acquire a connection via acquire() (blocks if none available) and
 * release it back via release(). Prevents the overhead of creating a new
 * database connection per request under concurrent load.
 *
 * \note Pool size should be tuned to the PostgreSQL max_connections setting.
 *       acquire() blocks indefinitely if all connections are held — there is
 *       no timeout. Use PooledConnection (RAII wrapper) to avoid leaks.
 */
class PostgresConnectionPool
{
public:
    PostgresConnectionPool(const std::string &connStr, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
        {
            try
            {
                _pool.push(std::make_unique<pqxx::connection>(connStr));
            }
            catch (std::exception &e)
            {
                std::cout << "connection error " << e.what() << std::endl;
                throw "invalid connection pool";
            }
        }
    }

    std::unique_ptr<pqxx::connection> acquire()
    {
        std::unique_lock lock(_mutex);
        _condVar.wait(lock, [&]
                      { return !_pool.empty(); });

        auto conn = std::move(_pool.front());
        _pool.pop();
        return conn;
    }

    void release(std::unique_ptr<pqxx::connection> conn)
    {
        std::lock_guard lock(_mutex);
        _pool.push(std::move(conn));
        _condVar.notify_one();
    }

private:
    std::queue<std::unique_ptr<pqxx::connection>> _pool;
    std::mutex _mutex;
    std::condition_variable _condVar;
};

class PooledConnection
{
public:
    PooledConnection(PostgresConnectionPool &pool)
        : _pool(pool), _conn{_pool.acquire()}
    {
    }

    ~PooledConnection()
    {
        _pool.release(std::move(_conn));
    }

    pqxx::connection &get() { return *_conn; }

private:
    PostgresConnectionPool &_pool;
    std::unique_ptr<pqxx::connection> _conn;
};