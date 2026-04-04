#include "StressTester.h"
#include "../Services/UserService.h"
#include "../Services/UserService.h"
#include "../Services/user_service.pb.h"
#include "../Services/user_service.grpc.pb.h"

#include <grpcpp/grpcpp.h>
#include <array>

constexpr std::array names = {
    "SMITH", "JONES", "WILLIAMS", "TAYLOR", "BROWN", "DAVIES", "EVANS", "WILSON", "THOMAS", "JOHNSON", "ROBERTS",
    "ROBINSON", "THOMPSON", "WRIGHT", "WALKER", "WHITE", "EDWARDS", "HUGHES", "GREEN", "HALL", "LEWIS", "HARRIS",
    "CLARKE", "PATEL", "JACKSON", "WOOD", "TURNER", "MARTIN", "COOPER", "HILL", "WARD", "MORRIS", "MOORE", "CLARK",
    "LEE", "KING", "BAKER", "HARRISON", "MORGAN", "ALLEN", "JAMES", "SCOTT", "PHILLIPS", "WATSON", "DAVIS", "PARKER",
    "PRICE", "BENNETT", "YOUNG", "GRIFFITHS", "MITCHELL", "KELLY", "COOK", "CARTER", "RICHARDSON", "BAILEY", "COLLINS",
    "BELL", "SHAW", "MURPHY", "MILLER", "COX", "RICHARDS", "KHAN", "MARSHALL", "ANDERSON", "SIMPSON", "ELLIS", "ADAMS",
    "SINGH", "BEGUM", "WILKINSON", "FOSTER", "CHAPMAN", "POWELL", "WEBB", "ROGERS", "GRAY", "MASON", "ALI", "HUNT", "HUSSAIN",
    "CAMPBELL", "MATTHEWS", "OWEN", "PALMER", "HOLMES", "MILLS", "BARNES", "KNIGHT", "LLOYD", "BUTLER", "RUSSELL", "BARKER",
    "FISHER", "STEVENS", "JENKINS", "MURRAY", "DIXON", "HARVEY"};

constexpr std::array surnames = {"ABBOTT", "ARMSTRONG", "ATKINS", "AUSTIN", "BALL", "BARBER", "BARRETT", "BATES",
                                 "BECK", "BERRY", "BISHOP", "BLACK", "BLAKE", "BOLTON", "BOND", "BOOTH", "BRADLEY", "BRADY", "BROOKS", "BURNETT", "BURTON",
                                 "BYRNE", "CARR", "CARROLL", "CHAMBERS", "CHANDLER", "COLE", "CONNOR", "CONNOR", "CROSS", "CURTIS", "DALE", "DALTON",
                                 "DANIELS", "DAWSON", "DAY", "DEAN", "DOUGLAS", "DUNCAN", "DUNN", "DYER", "EATON", "ELLISON", "EMERSON", "FERGUSON",
                                 "FINCH", "FLEMING", "FORD", "FRASER", "GARDNER", "GARRETT", "GIBSON", "GILBERT", "GILES", "GOODWIN", "GORDON",
                                 "GRANT", "GRAVES", "HALE", "HAMMOND", "HANCOCK", "HARDY", "HARPER", "HART", "HAWKINS", "HAYES", "HENDERSON", "HICKS",
                                 "HODGE", "HOPKINS", "HORTON", "HOWARD", "HUDSON", "HUNTER", "IRVING", "JARVIS", "JEFFERSON", "JOHNSTON", "JORDAN",
                                 "KELLEY", "KENT", "KERR", "LAMBERT", "LAWSON", "LITTLE", "LONG", "LUCAS", "MACDONALD", "MACKENZIE", "MANN", "MANNING",
                                 "MARLOW", "MAY", "MCCARTHY", "MCDONALD", "MCLEAN", "MEYER", "NASH", "NEAL", "NEWTON"};

void StressTester::testUserService()
{

     int threads = 50;

    std::vector<std::thread> pool;

    for (int i = 0; i < threads; ++i) {
        pool.emplace_back(worker, i);
    }

    // мониторинг
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "RPS ~ "
                  << success.load() << " success, "
                  << failed.load() << " failed"
                  << std::endl;

        success = 0;
        failed = 0;
    }

    for (auto& t : pool) t.join();
}

void StressTester::testFillUsers()
{
    if (connector->is_open())
    {
        pqxx::work txn(*connector);
        srand(time(0));

        for (size_t i = 0; i < 1000; ++i)
        {
            int nameIndex = std::rand() % names.size();
            ;
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

void StressTester::fillUpBalances()
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

void StressTester::worker()
{
    auto channel = grpc::CreateChannel("localhost:6868",
                                       grpc::InsecureChannelCredentials());

    auto stub = UserService::NewStub(channel);

    while (true)
    {
        user_service::LoginRequest request;
        request.mutable_user()->set_email("test@mail.com");
        request.mutable_user()->set_password("1234");

        user_service::LoginResponse response;
        grpc::ClientContext context;

        auto status = stub->Login(&context, request, &response);

        if (status.ok())
        {
            success++;
        }
        else
        {
            failed++;
        }
    }
}

void utilDatabaseConnection()
{
    // try
    // {
    //     std::string dbCredentials = "host = " + host + " "
    //                                                    "port= " +
    //                                 port + " "
    //                                        "dbname= " +
    //                                 dbName + " "
    //                                          "user=betting_admin "
    //                                          "password=kiba";

    //     connector = std::make_unique<pqxx::connection>(dbCredentials.data());
    // }
    // catch (const std::exception &e)
    // {
    //     std::cerr << "Error: " << e.what() << std::endl;
    // }
}
