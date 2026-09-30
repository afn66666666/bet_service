# Bet Service

Микросервисный backend для спортивных ставок на **C++ и Go**. Проект объединяет авторизацию пользователей, выдачу и проверку JWT, обработку ставок и нагрузочное моделирование клиентских сессий. Сервисы общаются с клиентами через **gRPC**, контракты описаны в **Protocol Buffers**, данные хранятся в **PostgreSQL**.

## Архитектура

```text
                         load_service / gRPC-клиент
                            │               │
                  email + пароль       ставка + JWT
                            │               │
                            ▼               ▼
                     login_service      bet_service
                         C++17              Go
                      порт 50051        порт 50052
                            │               │
                            ▼               ▼
                         PostgreSQL: betting_db
                              users + bets
```

При запуске с приведёнными ниже настройками Docker сервис авторизации доступен на `localhost:6868`, сервис ставок — на `localhost:6869`.

| Компонент | Назначение | Основные технологии |
|---|---|---|
| `login_service` | Проверка учётных данных и выдача access token | C++17, gRPC, libpqxx, libsodium, jwt-cpp |
| `bet_service` | Проверка токена, работа с балансом и запись ставок | Go, gRPC, pgx, golang-jwt |
| `load_service` | Моделирование пользователей и измерение нагрузки | Go, goroutines, gRPC, атомарные счётчики |
| PostgreSQL | Хранение пользователей, балансов и ставок | Транзакции и блокировки строк |

## Как это работает

### Авторизация

Клиент отправляет email и пароль в `login_service`. Сервис получает запись пользователя из PostgreSQL и проверяет хеш пароля через `crypto_pwhash_str_verify` из libsodium. При успешной проверке клиент получает идентификатор пользователя и подписанный JWT.

Токен подписывается алгоритмом **RS256** и действует **15 минут**. В нём передаются:

| Claim | Значение |
|---|---|
| `sub` | Идентификатор пользователя |
| `iss` | `login_service` |
| `aud` | `bet_service` |
| `iat` | Время выдачи |
| `exp` | Время окончания действия |

Асинхронный gRPC-сервер использует completion queues и 16 рабочих потоков. Доступ к PostgreSQL организован через пул из 50 соединений; SQL-запросы выполняются внутри обработчиков. RSA-ключи загружаются из файлов при старте.

### Размещение ставки

Клиент вызывает `bet_service.BettingService/PlaceBet` и передаёт токен в gRPC metadata:

```text
authorization: Bearer <access_token>
```

Interceptor проверяет подпись RS256, издателя, аудиторию, срок действия и идентификатор пользователя в `sub`. Обработчик ставки получает идентификатор из проверенного токена. Поле `user_id` в запросе сохраняется в контракте, но для выбора счёта используется `sub`.

В режиме работы с базой ставка обрабатывается в одной транзакции:

1. Сервис читает баланс пользователя через `SELECT ... FOR UPDATE`.
2. Проверяет, достаточно ли средств для ставки.
3. Уменьшает баланс на сумму ставки.
4. Записывает событие, сумму и выбранный исход в таблицу `bets`.
5. Фиксирует транзакцию и возвращает новый баланс.

Блокировка строки упорядочивает параллельные изменения одного баланса. При выходе из обработчика без успешного commit транзакция откатывается.

### Режимы обработки ставок

Режим задаётся константой `noopMode` в `Services/bet_service/server.go` и выбирается перед сборкой.

| Значение | Поведение |
|---|---|
| `true` — NOOP, включён по умолчанию | Проверка JWT и ответ `success: true`, `new_balance: 1000` без обращения к PostgreSQL |
| `false` — работа с БД | Проверка баланса, списание и запись ставки в транзакции |

NOOP позволяет измерять обработку gRPC-запросов и проверку токенов отдельно от базы данных. При сравнении результатов нагрузки следует указывать выбранный режим.

## API

Контракт ставок находится в [`Services/bet_service/bet_service.proto`](Services/bet_service/bet_service.proto).

**`PlaceBetRequest`**

| Поле | Тип Protobuf | Содержание |
|---|---|---|
| `user_id` | `int64` | Идентификатор в клиентском запросе; сервер использует пользователя из JWT |
| `event_id` | `string` | Идентификатор спортивного события |
| `amount` | `double` | Сумма ставки |
| `outcome` | `string` | Выбранный исход, например `home`, `draw` или `away` |

**`PlaceBetResponse`** содержит `success` (`bool`), `error` (`string`) и `new_balance` (`double`). Например, при недостаточном балансе сервис возвращает `success: false` и `error: "insufficient balance"`. Ошибки авторизации возвращаются со статусом gRPC `Unauthenticated`.

Версионированный контракт авторизации расположен в [`Services/proto/login/v1/login.proto`](Services/proto/login/v1/login.proto): метод `login.v1.LoginService/Login` принимает `email` и `password`, возвращает `user_id` и `access_token`.

**Совместимость контрактов:** Go-генератор нагрузки использует `login.v1.LoginService`. C++-обработчик авторизации и его CMake-конфигурация используют `user_service.UserService`, вложенное поле `user` в запросе и поле `token` в ответе. Для совместного запуска клиент и сервер авторизации должны использовать один контракт; CMake ожидает файл `Services/login_service/user_service.proto`.

## Структура репозитория

```text
Services/
├── login_service/          # C++-сервис авторизации, пул БД и подпись JWT
│   ├── AsyncUserService.*  # Completion queues и рабочие потоки
│   ├── CallHandler.*       # Обработка Login
│   ├── ConnectionPool.h    # Пул соединений PostgreSQL
│   └── JWTTokenSigner.*    # Формирование access token
├── bet_service/            # Go-сервис ставок
│   ├── auth.go             # Проверка JWT и gRPC interceptor
│   ├── server.go           # PlaceBet и переключение NOOP
│   ├── db.go               # Создание пула PostgreSQL
│   └── bet_service.proto   # Контракт ставок
├── load_service/           # Генератор нагрузки и клиентские сценарии
│   ├── main.go             # Параметры запуска
│   ├── virtual_user.go     # Жизненный цикл виртуального пользователя
│   ├── login_simulator.go  # Клиент авторизации и метрики
│   └── betgun.go           # Отправка ставок и метрики
└── proto/login/v1/         # Версионированный контракт Login
```

## Настройка и запуск

Команды ниже предназначены для PowerShell и выполняются из корня репозитория, если не указано иное. Для контейнерного запуска нужны Docker, OpenSSL для подготовки ключей и PostgreSQL для авторизации и обработки ставок с сохранением данных.

### PostgreSQL

Оба серверных приложения используют базу `betting_db` по адресу `host.docker.internal:5432`. Параметры подключения заданы в `Services/login_service/main.cpp` и `Services/bet_service/main.go`; перед сборкой их следует согласовать со своей базой.

Сервисы обращаются к следующим столбцам:

| Таблица | Столбцы |
|---|---|
| `users` | `id`, `email`, `password`, `balance` |
| `bets` | `user_id`, `event_id`, `amount`, `outcome` |

В `users.password` хранится строковый хеш, совместимый с libsodium. Для клиентского сценария нужны заранее созданные пользователи с известными тестовыми паролями и балансами.

### RSA-ключи

Для первого запуска создайте пару ключей. Если ключи уже подготовлены, используйте существующие файлы.

```powershell
New-Item -ItemType Directory -Force secrets
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:3072 -out secrets/jwt-private.pem
openssl pkey -in secrets/jwt-private.pem -pubout -out secrets/jwt-public.pem
```

`login_service` получает оба ключа, `bet_service` — публичный ключ той же пары. Каталог `secrets/` исключён из Git.

### Сервис авторизации

Dockerfile собирает C++-приложение с помощью CMake и генерирует protobuf-код. Для этой сборки требуется `user_service.proto`, соответствующий C++-обработчику из раздела о совместимости контрактов.

```powershell
docker build -t login-service ./Services/login_service
docker run --rm --name login-service -p 6868:50051 `
  -v "${PWD}/secrets/jwt-public.pem:/run/secrets/jwt-public.pem:ro" `
  -v "${PWD}/secrets/jwt-private.pem:/run/secrets/jwt-private.pem:ro" `
  -e JWT_PUBLIC_KEY_PATH=/run/secrets/jwt-public.pem `
  -e JWT_PRIVATE_KEY_PATH=/run/secrets/jwt-private.pem `
  login-service
```

### Сервис ставок

В отдельном терминале:

```powershell
docker build -t bet-service ./Services/bet_service
docker run --rm --name bet-service -p 6869:50052 `
  -v "${PWD}/secrets/jwt-public.pem:/run/secrets/jwt-public.pem:ro" `
  -e JWT_PUBLIC_KEY_PATH=/run/secrets/jwt-public.pem `
  bet-service
```

Dockerfile генерирует Go-код из `bet_service.proto` и собирает сервер. В NOOP-режиме сервису ставок достаточно публичного ключа; при `noopMode = false` требуется также доступ к PostgreSQL. Клиентские подключения в этих примерах используют gRPC без TLS.

## Нагрузочное моделирование

`load_service` запускает виртуальных пользователей в отдельных goroutines. Каждый пользователь авторизуется, сохраняет сессию и последовательно отправляет ставки с небольшим случайным разбросом интервалов. При ответе `Unauthenticated` пользователь проходит авторизацию повторно. Deadline каждого RPC — 2 секунды.

Сценарий отправляет ставки на `event_1` с суммой `1.0`, случайно выбирая `home`, `draw` или `away`.

Учётные данные читаются из JSON-файла:

```json
[
  { "email": "player1@example.com", "password": "test-password-1" },
  { "email": "player2@example.com", "password": "test-password-2" }
]
```

Эти записи должны соответствовать пользователям в базе данных.

### Подготовка Go-клиента

Нужны Go версии, указанной в `Services/load_service/go.mod` (1.25.12), `protoc` и плагины `protoc-gen-go`, `protoc-gen-go-grpc` в `PATH`. Сгенерированные файлы создаются локально и исключены из Git.

```powershell
New-Item -ItemType Directory -Force Services/bet_service/proto
protoc -I Services/bet_service --go_out=Services/bet_service/proto --go_opt=paths=source_relative --go-grpc_out=Services/bet_service/proto --go-grpc_opt=paths=source_relative Services/bet_service/bet_service.proto

New-Item -ItemType Directory -Force Services/load_service/proto
protoc -I Services/proto --go_out=Services/load_service/proto --go_opt=paths=source_relative --go-grpc_out=Services/load_service/proto --go-grpc_opt=paths=source_relative Services/proto/login/v1/login.proto
```

После запуска сервера авторизации с контрактом `login.v1` и сервиса ставок:

```powershell
Set-Location Services/load_service
go run main.go credentials.go login_simulator.go virtual_user.go betgun.go -credentials credentials.json -users 2 -bets-per-second 8 -duration 30s -ramp-up 5s
```

В команде явно перечислены Go-файлы, поскольку в том же каталоге находятся исходники C++-клиента.

| Параметр | По умолчанию | Назначение |
|---|---|---|
| `-login-addr` | `localhost:6868` | Адрес авторизации |
| `-bet-addr` | `localhost:6869` | Адрес сервиса ставок |
| `-credentials` | `credentials.json` | Файл учётных данных |
| `-users` | `0` | Число пользователей; `0` выбирает все записи |
| `-bets-per-second` | `8` | Целевая частота ставок на пользователя |
| `-duration` | `10s` | Общая длительность сценария, включая разгон |
| `-ramp-up` | `5s` | Интервал постепенного запуска пользователей |

Раз в секунду клиент выводит приблизительный RPS, количество успешных запросов, отказов и ошибок, а также среднее время ответа отдельно для авторизации и ставок. Произведение числа пользователей на `-bets-per-second` задаёт ориентир нагрузки; фактическая частота зависит также от времени ответов сервера. Сценарий завершается по истечении заданного времени или по `Ctrl+C`.
