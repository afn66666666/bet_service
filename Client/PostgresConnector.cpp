#include "PostgresConnector.h"

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

void PostgresConnector::testFillUsers()
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

void PostgresConnector::fillUpBalances()
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
