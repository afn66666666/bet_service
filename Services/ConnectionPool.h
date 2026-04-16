#pragma once
#include <pqxx/pqxx>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

class ConnectionPool
{

    ConnectionPool(const std::string &connStr, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
        {
            _pool.push(std::make_unique<pqxx::connection>(connStr));
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