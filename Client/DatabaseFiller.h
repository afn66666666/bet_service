#include "pqxx/pqxx"
#include "MetaData.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <memory>
#include <sodium.h>
/*!
 * \brief Utility for seeding the betting_db with test data.
 *
 * testFillUsers() — inserts the 1000 MetaData users (email + Argon2id-hashed password),
 *                   index-aligned with emails[]/passwords[] so LoginSimulator can log in.
 * fillUpBalances() — assigns a random balance (5000–100000) to every user row.
 *
 * Used once during initial setup; not part of the request path.
 */
class DatabaseFiller
{
public:
    DatabaseFiller()
    {
        connector = std::make_unique<pqxx::connection>(
            "host=localhost port=5432 dbname=betting_db user=betting_admin password=kiba");
    }

    void testFillUsers()
    {
        if (sodium_init() < 0)
            throw std::runtime_error("libsodium init failed");

        if (connector->is_open())
        {
            pqxx::work txn(*connector);

            // Seed straight from the MetaData arrays (emails[i] <-> passwords[i]),
            // so LoginSimulator can authenticate by the same index.
            for (size_t i = 0; i < emails.size(); ++i)
            {
                std::string pass = passwords[i];

                // hashing password (Argon2id; salt + params embedded in the string)
                char hashed[crypto_pwhash_STRBYTES];
                if (crypto_pwhash_str(
                        hashed, pass.c_str(), pass.size(),
                        crypto_pwhash_OPSLIMIT_INTERACTIVE,   // t
                        crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) // m  (~64 MiB)
                    throw std::runtime_error("password hashing failed (out of memory)");

                txn.exec_params(
                    "INSERT INTO users(email, password) VALUES ($1, $2) "
                    "ON CONFLICT (email) DO NOTHING",
                    emails[i],
                    std::string(hashed));
            }
            txn.commit();
        }
    }

    void fillUpBalances()
    {
        if (connector->is_open())
        {
            pqxx::work work(*connector);

            pqxx::result r = work.exec("SELECT COUNT(*) FROM users");
            auto usersCount = r[0][0].as<size_t>();

            work.exec(
                "UPDATE users SET balance = balance + (5000 + random() * (100000 - 5000))::numeric(18,2);");

            work.commit();

            std::cout << "balances updated : " << usersCount << " rows" << std::endl;
        }
    }

private:
    std::unique_ptr<pqxx::connection> connector;
};