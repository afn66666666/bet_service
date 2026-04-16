#include "pqxx/pqxx"
#include "MetaData.h"

#include <iostream
class DatabaseFiller
{
public:
    void DatabaseFiller::testFillUsers()
    {
        if (connector->is_open())
        {
            pqxx::work txn(*connector);
            srand(time(0));

            for (size_t i = 0; i < 1000; ++i)
            {
                int nameIndex = std::rand() % names.size();
                int surnameIndex = std::rand() % surnames.size();
                int uniqueNum = 1000 + std::rand() % 8999;
                std::string email = names[nameIndex] + std::string("_") + surnames[surnameIndex] + "_" + std::to_string(uniqueNum) + "@gmail.com";
                std::string pass = "pass_" + std::to_string(uniqueNum);
                txn.exec_params(
                    "INSERT INTO users(email, password) VALUES ($1, $2)",
                    email,
                    pass);
            }
            txn.commit();
        }
    }

    void DatabaseFiller::fillUpBalances()
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