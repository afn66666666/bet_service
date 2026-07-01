# Docker_gRPC — Pet Project: Highload Sports Betting Auth Service

## Role

You are a senior C++ backend developer. Priorities in order:
1. Correct, working code
2. Performance and efficiency
3. Architecture and design patterns
4. Code guidelines and readability

## Code Rules

- No emojis or emoticons anywhere in code, comments, or output
- Follow existing code style in the file being edited
- Prefer minimal, precise changes — do not refactor beyond the task scope
- C++17

## Project Overview

Distributed microservice system for high-load user authentication in a sports betting platform.
Designed to handle thousands of concurrent login requests via async gRPC.

**Stack:** C++17, gRPC, Protobuf, PostgreSQL (libpqxx), Boost.ASIO, Docker

## Architecture

```
Client (StressTester)
    └── gRPC async calls → localhost:6868
            └── UserService (port 50051, Docker container)
                    └── ConnectionPool → PostgreSQL (betting_db)

Server (port 8080)
    └── IResponser interface
            ├── GrpcResponserImpl   (active)
            └── TcpResponserImpl    (alternative, ASIO-based)
```

### Components

| Directory | Binary | Role |
|-----------|--------|------|
| `Services/` | `user_service` | Auth microservice: Login RPC, DB connection pool |
| `Server/` | (WIP) | Gateway server, pluggable transport via IResponser |
| `Client/` | `client` | Async stress tester: 8 threads × 750 req = 6000 req/run |

### Key Classes

| Class | File | Role |
|-------|------|------|
| `UserService` | `Services/UserService.h/.cpp` | gRPC service impl, Login handler |
| `ConnectionPool` | `Services/ConnectionPool.h` | Thread-safe PostgreSQL connection pool (50 conn) |
| `PooledConnection` | `Services/ConnectionPool.h` | RAII wrapper: auto-releases connection on destruction |
| `StressTester` | `Client/StressTester.h/.cpp` | Async gRPC load tester, per-thread completion queues |
| `IResponser` | `Server/IResponser.h` | Abstract transport interface |
| `GrpcResponserImpl` | `Server/GrpsResponserImpl.h` | gRPC server on port 8080 |
| `TcpResponserImpl` | `Server/TcpResponserImpl.h` | Raw TCP/ASIO alternative transport |

### Proto Files

- `Services/user_service.proto` — UserService.Login RPC (email/pass → token + user_id)
- `Server/server_config.proto` — Server.SignIn RPC (User message → success flag)

## Build

```bash
# Services (Linux/Docker)
cd Services && mkdir build && cd build
cmake .. && make

# Client (Windows, vcpkg)
cd Client && mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=L:/programming/cpp/vcpkg/scripts/buildsystems/vcpkg.cmake ..
cmake --build .

# Docker
docker build -t user_service ./Services
docker run -p 6868:50051 --name user_service_container -t user_service
```

## Database

```
host=host.docker.internal port=5432 dbname=betting_db user=betting_admin password=kiba
```

```sql
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    email VARCHAR UNIQUE NOT NULL,
    password VARCHAR NOT NULL,
    balance NUMERIC(18,2) DEFAULT 0
);
```

Seeding: use `DatabaseFiller::testFillUsers()` + `fillUpBalances()` in Client.

## Known Issues (Architecture Debt)

1. **NOOP_FLAG macro** — `UserService::Login` returns `OK` immediately without touching DB
2. **generateLoginData hardcoded** — random logic commented out, always uses `emails[70]`
3. **StressTester infinite loop** — `run()` never terminates; `_workers.join()` unreachable
4. **ConnectionPool::acquire() no timeout** — blocks indefinitely under DB exhaustion
5. **throw string literal** — `throw "invalid connection pool"` should be `std::runtime_error`
6. **refreshCounters() race** — resets atomics while workers still incrementing them
7. **Plaintext passwords** — no hashing (bcrypt/argon2 needed for production)
8. **Hardcoded credentials** — DB password in source; should be env vars
9. **No TLS** — insecure gRPC credentials everywhere
10. **Token trivial** — `"token_" + user_id` is not a real JWT

## Design Patterns Used

- **Strategy** — `IResponser` pluggable transport (gRPC vs raw TCP)
- **Object Pool** — `ConnectionPool` with RAII `PooledConnection`
- **Async Completion Queue** — gRPC `PrepareAsync` + `CompletionQueue` per thread
