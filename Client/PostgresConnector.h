#include "pqxx/pqxx"

#include <iostream>
#include <memory>

class PostgresConnector
{
public:
    PostgresConnector(const std::string &host, const std::string &port, const std::string &dbName)
    {
        try
        {
            std::string dbCredentials = "host = " + host + " " 
                                        "port= " + port + " " 
                                        "dbname= " + dbName + " " 
                                        "user=betting_admin "
                                        "password=kiba";

            connector = std::make_unique<pqxx::connection>(dbCredentials.data());
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void testFillUsers();
    void fillUpBalances();
private:
std::unique_ptr<pqxx::connection> connector;
};
